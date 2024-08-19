#include "candle_api.h"
#include "libusb.h"
#include "list.h"
#include "fifo.h"
#include "gs_usb_def.h"
#include <stdlib.h>
#include <string.h>
#include <threads.h>

static const size_t dlc2len[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};
static struct libusb_context *ctx = NULL;
static LIST_HEAD(device_list);
size_t open_devs;
thrd_t event_thread;
static bool event_thread_run;

struct candle_channel_handle {
    bool is_start;
    enum candle_mode mode;
    fifo_t *rx_fifo;
    cnd_t rx_cnd;
    mtx_t rx_cond_mtx;
};

struct candle_device_handle {
    struct list_head list;
    struct candle_device *device;
    struct libusb_device *usb_device;
    struct libusb_device_handle *usb_device_handle;
    struct libusb_transfer *rx_transfer;
    size_t rx_size;
    uint8_t in_ep;
    uint8_t out_ep;
    struct candle_channel_handle channels[];
};

static void clean_device_list(void) {
    struct candle_device_handle *pos;
    struct candle_device_handle *n;
    list_for_each_entry_safe(pos, n, &device_list, list) {
        candle_close_device_unsafe(pos->device);
        list_del(&pos->list);
        libusb_unref_device(pos->usb_device);
        free(pos->device);
        free(pos);
    }
}

static int event_thread_func(void *arg) {
    while (event_thread_run)
        libusb_handle_events(arg);

    return 0;
}

static void after_libusb_open_hook(void) {
    open_devs++;
    if (open_devs == 1) {
        // start event loop
        event_thread_run = true;
        thrd_create(&event_thread, event_thread_func, ctx);
    }
}

static void before_libusb_close_hook(void) {
    open_devs--;
    if (open_devs == 0) {
        // stop event loop
        event_thread_run = false;
    }
}

static void after_libusb_close_hook(void) {
    if (open_devs == 0) {
        // join event loop
        int res;
        thrd_join(event_thread, &res);
    }
}

static void receive_bulk_callback(struct libusb_transfer *transfer) {
    struct candle_device_handle *handle = transfer->user_data;

    struct gs_host_frame *hf;
    switch (transfer->status) {
        case LIBUSB_TRANSFER_CANCELLED:
            free(transfer->buffer);
            libusb_free_transfer(handle->rx_transfer);
            handle->rx_transfer = NULL;
            break;
        case LIBUSB_TRANSFER_NO_DEVICE:
            handle->device->is_connected = false;
            free(transfer->buffer);
            libusb_free_transfer(handle->rx_transfer);
            handle->rx_transfer = NULL;
            break;
        case LIBUSB_TRANSFER_COMPLETED:
            hf = (struct gs_host_frame *) transfer->buffer;
            fifo_put(handle->channels[hf->channel].rx_fifo, hf);
            mtx_lock(&handle->channels[hf->channel].rx_cond_mtx);
            cnd_signal(&handle->channels[hf->channel].rx_cnd);
            mtx_unlock(&handle->channels[hf->channel].rx_cond_mtx);
            libusb_submit_transfer(transfer);
            break;
        default:
            libusb_submit_transfer(transfer);
    }
}

static void transmit_bulk_callback(struct libusb_transfer *transfer) {
    struct candle_device_handle *handle = transfer->user_data;

    switch (transfer->status) {
        case LIBUSB_TRANSFER_NO_DEVICE:
            handle->device->is_connected = false;
            free(transfer->buffer);
            libusb_free_transfer(transfer);
            break;
        case LIBUSB_TRANSFER_COMPLETED:
        case LIBUSB_TRANSFER_CANCELLED:
            free(transfer->buffer);
            libusb_free_transfer(transfer);
            break;
        default:
            libusb_submit_transfer(transfer);
    }
}

bool candle_initialize_unsafe(void) {
    if (ctx == NULL && libusb_init_context(&ctx, NULL, 0) == LIBUSB_SUCCESS)
        return true;
    return false;
}

bool candle_finalize_unsafe(void) {
    clean_device_list();
    libusb_exit(ctx);
    ctx = NULL;
    return true;
}

bool candle_list_device_unsafe(struct candle_device **devices, size_t *size) {
    if (ctx == NULL) return false;

    struct libusb_device **usb_device_list;
    ssize_t count = libusb_get_device_list(ctx, &usb_device_list);
    if (count < 0) return false;

    int rc;
    size_t actual_size = 0;
    LIST_HEAD(new_device_list);
    struct candle_device_handle *pos;
    struct candle_device_handle *n;
    for (size_t i = 0; i < (size_t) count; ++i) {
        struct libusb_device *dev = usb_device_list[i];
        struct libusb_device_descriptor desc;

        rc = libusb_get_device_descriptor(dev, &desc);
        if (rc != LIBUSB_SUCCESS) continue;

        if ((desc.idVendor == 0x1d50 && desc.idProduct == 0x606f) ||
            (desc.idVendor == 0x1209 && desc.idProduct == 0x2323) ||
            (desc.idVendor == 0x1cd2 && desc.idProduct == 0x606f) ||
            (desc.idVendor == 0x16d0 && desc.idProduct == 0x10b8) ||
            (desc.idVendor == 0x16d0 && desc.idProduct == 0x0f30)) {

            struct candle_device *target_candle_device = NULL;
            list_for_each_entry_safe(pos, n, &device_list, list) {
                if (pos->usb_device == dev) {
                    list_del(&pos->list);
                    list_add_tail(&pos->list, &new_device_list);
                    target_candle_device = pos->device;
                }
            }

            if (target_candle_device == NULL) {
                struct libusb_device_handle *dev_handle;
                rc = libusb_open(dev, &dev_handle);
                after_libusb_open_hook();
                if (rc != LIBUSB_SUCCESS) goto handle_error;

                // read usb descriptions
                struct candle_device candle_dev;
                candle_dev.is_connected = true;
                candle_dev.is_open = false;
                candle_dev.vendor_id = desc.idVendor;
                candle_dev.product_id = desc.idProduct;
                libusb_get_string_descriptor_ascii(dev_handle, desc.iManufacturer, (uint8_t *) candle_dev.manufacturer,
                                                   256);
                libusb_get_string_descriptor_ascii(dev_handle, desc.iProduct, (uint8_t *) candle_dev.product, 256);
                libusb_get_string_descriptor_ascii(dev_handle, desc.iSerialNumber, (uint8_t *) candle_dev.serial_number,
                                                   256);

                // send host config
                struct gs_host_config hconf;
                hconf.byte_order = 0x0000beef;
                rc = libusb_control_transfer(dev_handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                                                         LIBUSB_RECIPIENT_INTERFACE, GS_USB_BREQ_HOST_FORMAT, 1, 0,
                                             (uint8_t *) &hconf, sizeof(hconf), 1000);
                if (rc < LIBUSB_SUCCESS) goto handle_error;

                // read device config
                struct gs_device_config dconf;
                rc = libusb_control_transfer(dev_handle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                                                         LIBUSB_RECIPIENT_INTERFACE, GS_USB_BREQ_DEVICE_CONFIG, 1, 0,
                                             (uint8_t *) &dconf, sizeof(dconf), 1000);
                if (rc < LIBUSB_SUCCESS) goto handle_error;
                uint8_t channel_count = dconf.icount + 1;
                candle_dev.channel_count = channel_count;
                candle_dev.software_version = dconf.sw_version;
                candle_dev.hardware_version = dconf.hw_version;

                // read channel info
                struct candle_device *new_candle_device = malloc(
                        sizeof(struct candle_device) + channel_count * sizeof(struct candle_channel));
                if (new_candle_device == NULL) goto handle_error;
                memcpy(new_candle_device, &candle_dev, sizeof(candle_dev));
                for (uint8_t j = 0; j < channel_count; ++j) {
                    struct gs_device_bt_const bt_const;
                    rc = libusb_control_transfer(dev_handle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                                                             LIBUSB_RECIPIENT_INTERFACE, GS_USB_BREQ_BT_CONST, j, 0,
                                                 (uint8_t *) &bt_const, sizeof(bt_const), 1000);
                    if (rc < LIBUSB_SUCCESS) {
                        free(new_candle_device);
                        goto handle_error;
                    }
                    new_candle_device->channels[j].feature = bt_const.feature;
                    new_candle_device->channels[j].clock_frequency = bt_const.fclk_can;
                    new_candle_device->channels[j].bit_timing_const.nominal.tseg1_min = bt_const.tseg1_min;
                    new_candle_device->channels[j].bit_timing_const.nominal.tseg1_max = bt_const.tseg1_max;
                    new_candle_device->channels[j].bit_timing_const.nominal.tseg2_min = bt_const.tseg2_min;
                    new_candle_device->channels[j].bit_timing_const.nominal.tseg2_max = bt_const.tseg2_max;
                    new_candle_device->channels[j].bit_timing_const.nominal.sjw_max = bt_const.sjw_max;
                    new_candle_device->channels[j].bit_timing_const.nominal.brp_min = bt_const.brp_min;
                    new_candle_device->channels[j].bit_timing_const.nominal.brp_max = bt_const.brp_max;
                    new_candle_device->channels[j].bit_timing_const.nominal.brp_inc = bt_const.brp_inc;

                    if (desc.idVendor == 0x1d50 && desc.idProduct == 0x606f &&
                        !strcmp(new_candle_device->manufacturer, "LinkLayer Labs") &&
                        !strcmp(new_candle_device->product, "CANtact Pro") && dconf.sw_version <= 2)
                        new_candle_device->channels[j].feature |=
                                CANDLE_FEATURE_REQ_USB_QUIRK_LPC546XX | CANDLE_FEATURE_QUIRK_BREQ_CANTACT_PRO;

                    if (!(dconf.sw_version > 1 && new_candle_device->channels[j].feature & CANDLE_FEATURE_IDENTIFY))
                        new_candle_device->channels[j].feature &= ~CANDLE_FEATURE_IDENTIFY;

                    if (bt_const.feature & CANDLE_FEATURE_FD && bt_const.feature & CANDLE_FEATURE_BT_CONST_EXT) {
                        struct gs_device_bt_const_extended bt_const_ext;
                        rc = libusb_control_transfer(dev_handle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                                                                 LIBUSB_RECIPIENT_INTERFACE, GS_USB_BREQ_BT_CONST_EXT,
                                                     j, 0, (uint8_t *) &bt_const_ext, sizeof(bt_const_ext), 1000);
                        if (rc < LIBUSB_SUCCESS) {
                            free(new_candle_device);
                            goto handle_error;
                        }
                        new_candle_device->channels[j].bit_timing_const.data.tseg1_min = bt_const_ext.dtseg1_min;
                        new_candle_device->channels[j].bit_timing_const.data.tseg1_max = bt_const_ext.dtseg1_max;
                        new_candle_device->channels[j].bit_timing_const.data.tseg2_min = bt_const_ext.dtseg2_min;
                        new_candle_device->channels[j].bit_timing_const.data.tseg2_max = bt_const_ext.dtseg2_max;
                        new_candle_device->channels[j].bit_timing_const.data.sjw_max = bt_const_ext.dsjw_max;
                        new_candle_device->channels[j].bit_timing_const.data.brp_min = bt_const_ext.dbrp_min;
                        new_candle_device->channels[j].bit_timing_const.data.brp_max = bt_const_ext.dbrp_max;
                        new_candle_device->channels[j].bit_timing_const.data.brp_inc = bt_const_ext.dbrp_inc;
                    } else
                        memset(&new_candle_device->channels[j].bit_timing_const.data, 0,
                               sizeof(struct candle_bit_timing_const));
                }

                // create internal handle
                struct candle_device_handle *handle = malloc(
                        sizeof(struct candle_device_handle) + channel_count * sizeof(struct candle_channel_handle));
                if (handle == NULL) {
                    free(new_candle_device);
                    goto handle_error;
                }
                handle->device = new_candle_device;
                handle->rx_transfer = NULL;
                new_candle_device->handle = handle;

                // find bulk endpoints
                handle->in_ep = 0;
                handle->out_ep = 0;
                struct libusb_config_descriptor *conf_desc;
                rc = libusb_get_active_config_descriptor(dev, &conf_desc);
                if (rc != LIBUSB_SUCCESS) {
                    free(handle);
                    free(new_candle_device);
                    goto handle_error;
                }
                for (uint8_t j = 0; j < conf_desc->interface[0].altsetting[0].bNumEndpoints; ++j) {
                    uint8_t ep_address = conf_desc->interface[0].altsetting[0].endpoint[j].bEndpointAddress;
                    uint8_t ep_type = conf_desc->interface[0].altsetting[0].endpoint[j].bmAttributes;
                    if (ep_address & LIBUSB_ENDPOINT_IN && ep_type & LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK)
                        handle->in_ep = ep_address;
                    if (!(ep_address & LIBUSB_ENDPOINT_IN) && ep_type & LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK)
                        handle->out_ep = ep_address;
                }
                if (!(handle->in_ep && handle->out_ep)) {
                    free(handle);
                    free(new_candle_device);
                    goto handle_error;
                }

                // new target device
                handle->usb_device = libusb_ref_device(dev);
                list_add_tail(&handle->list, &new_device_list);
                target_candle_device = new_candle_device;

                handle_error:
                before_libusb_close_hook();
                libusb_close(dev_handle);
                after_libusb_close_hook();
            }

            // put device to output list
            if (target_candle_device != NULL) {
                actual_size++;
                if (actual_size <= *size) {
                    devices[actual_size - 1] = target_candle_device;
                }
            }
        }
    }
    libusb_free_device_list(usb_device_list, 1);

    // clean old device
    clean_device_list();

    // update device list
    list_splice_tail_init(&new_device_list, &device_list);

    *size = actual_size;
    return true;
}

bool candle_open_device_unsafe(struct candle_device *device) {
    struct candle_device_handle *handle = device->handle;

    if (device->is_open)
        return false;

    int rc = libusb_open(handle->usb_device, &handle->usb_device_handle);
    if (rc != LIBUSB_SUCCESS) return false;

    after_libusb_open_hook();

    rc = libusb_set_auto_detach_kernel_driver(handle->usb_device_handle, 1);
    if (rc != LIBUSB_SUCCESS && rc != LIBUSB_ERROR_NOT_SUPPORTED) return false;

    rc = libusb_claim_interface(handle->usb_device_handle, 0);
    if (rc != LIBUSB_SUCCESS) return false;

    // reset channel
    for (int i = 0; i < device->channel_count; ++i) {
        candle_reset_channel_unsafe(device, i);
    }

    // calculate rx size
    struct gs_host_frame *hf;
    size_t hf_size_rx;
    handle->rx_size = 0;
    for (int i = 0; i < device->channel_count; ++i) {
        if (device->channels[i].feature & CANDLE_FEATURE_FD) {
            if (device->channels[i].feature & CANDLE_FEATURE_HW_TIMESTAMP)
                hf_size_rx = struct_size(hf, canfd_ts, 1);
            else
                hf_size_rx = struct_size(hf, canfd, 1);
        } else {
            if (device->channels[i].feature & CANDLE_FEATURE_HW_TIMESTAMP)
                hf_size_rx = struct_size(hf, classic_can_ts, 1);
            else
                hf_size_rx = struct_size(hf, classic_can, 1);
        }
        handle->rx_size = max(handle->rx_size, hf_size_rx);
    }

    // alloc transfer
    handle->rx_transfer = libusb_alloc_transfer(0);
    if (handle->rx_transfer == NULL) {
        return false;
    }

    // frame buffer
    uint8_t *fb = malloc(handle->rx_size);
    if (fb == NULL) {
        libusb_free_transfer(handle->rx_transfer);
        handle->rx_transfer = NULL;
        return false;
    }

    // fill transfer
    libusb_fill_bulk_transfer(handle->rx_transfer, handle->usb_device_handle, handle->in_ep,
                              fb, (int) handle->rx_size, receive_bulk_callback, handle, 1000);

    // submit transfer
    rc = libusb_submit_transfer(handle->rx_transfer);
    if (rc != LIBUSB_SUCCESS) {
        libusb_free_transfer(handle->rx_transfer);
        handle->rx_transfer = NULL;
        free(fb);
        return false;
    }

    device->is_open = true;
    return true;
}

bool candle_close_device_unsafe(struct candle_device *device) {
    struct candle_device_handle *handle = device->handle;

    if (!device->is_open)
        return false;

    // reset channel
    for (int i = 0; i < device->channel_count; ++i) {
        candle_reset_channel_unsafe(device, i);
    }

    // cancel transfer
    libusb_cancel_transfer(handle->rx_transfer);

    // release usb device
    libusb_release_interface(handle->usb_device_handle, 0);
    before_libusb_close_hook();
    libusb_close(handle->usb_device_handle);
    after_libusb_close_hook();
    device->is_open = false;
    return true;
}

bool candle_reset_channel_unsafe(struct candle_device *device, uint8_t channel) {
    struct candle_device_handle *handle = device->handle;

    if (!device->is_open)
        return false;

    struct gs_device_mode md = {.mode = 0};
    int rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_MODE, channel, 0, (uint8_t *) &md, sizeof(md), 1000);
    if (rc < LIBUSB_SUCCESS)
        return false;

    fifo_free(handle->channels[channel].rx_fifo);
    handle->channels[channel].rx_fifo = NULL;
    cnd_destroy(&handle->channels[channel].rx_cnd);
    mtx_destroy(&handle->channels[channel].rx_cond_mtx);

    handle->channels[channel].mode = CANDLE_MODE_NORMAL;
    handle->channels[channel].is_start = false;

    return true;
}

bool candle_start_channel_unsafe(struct candle_device *device, uint8_t channel, enum candle_mode mode) {
    struct candle_device_handle *handle = device->handle;

    if (!device->is_open)
        return false;

    struct gs_device_mode md = {.mode = 1, .flags = mode};
    int rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_MODE, channel, 0, (uint8_t *) &md, sizeof(md), 1000);
    if (rc < LIBUSB_SUCCESS)
        return false;

    // handle->rx_size <= 80
    handle->channels[channel].rx_fifo = fifo_create((char) handle->rx_size, 10);
    if (handle->channels[channel].rx_fifo == NULL)
        return false;

    cnd_init(&handle->channels[channel].rx_cnd);
    mtx_init(&handle->channels[channel].rx_cond_mtx, mtx_timed);

    handle->channels[channel].mode = mode;
    handle->channels[channel].is_start = true;

    return true;
}

bool candle_set_bit_timing_unsafe(struct candle_device *device, uint8_t channel, struct candle_bit_timing *bit_timing) {
    struct candle_device_handle *handle = device->handle;

    if (!device->is_open)
        return false;

    struct gs_device_bittiming bt = {.prop_seg = bit_timing->prop_seg, .phase_seg1 = bit_timing->phase_seg1, .phase_seg2 = bit_timing->phase_seg2, .sjw = bit_timing->sjw, .brp = bit_timing->brp};
    int rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_BITTIMING, channel, 0, (uint8_t *) &bt, sizeof(bt), 1000);
    if (rc < LIBUSB_SUCCESS)
        return false;

    return true;
}

bool candle_set_data_bit_timing_unsafe(struct candle_device *device, uint8_t channel, struct candle_bit_timing *bit_timing) {
    struct candle_device_handle *handle = device->handle;

    if (!device->is_open)
        return false;

    struct gs_device_bittiming bt = {.prop_seg = bit_timing->prop_seg, .phase_seg1 = bit_timing->phase_seg1, .phase_seg2 = bit_timing->phase_seg2, .sjw = bit_timing->sjw, .brp = bit_timing->brp};
    int rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_DATA_BITTIMING, channel, 0, (uint8_t *) &bt, sizeof(bt), 1000);
    if (rc < LIBUSB_SUCCESS)
        return false;
    return true;
}

bool candle_get_termination_unsafe(struct candle_device *device, uint8_t channel, bool *enable) {
    struct candle_device_handle *handle = device->handle;

    if (!device->is_open)
        return false;

    uint32_t state;
    int rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_GET_TERMINATION, channel, 0, (uint8_t *) &state, sizeof(state), 1000);
    if (rc < LIBUSB_SUCCESS)
        return false;

    if (state)
        *enable = true;
    else
        *enable = false;
    return true;
}

bool candle_set_termination_unsafe(struct candle_device *device, uint8_t channel, bool enable) {
    struct candle_device_handle *handle = device->handle;

    if (!device->is_open)
        return false;

    uint32_t state = enable ? 1 : 0;
    int rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_SET_TERMINATION, channel, 0, (uint8_t *) &state, sizeof(state), 1000);
    if (rc < LIBUSB_SUCCESS)
        return false;
    return true;
}

bool candle_get_state_unsafe(struct candle_device *device, uint8_t channel, struct candle_state *state) {
    struct candle_device_handle *handle = device->handle;

    if (!device->is_open)
        return false;

    struct gs_device_state st;
    int rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_GET_STATE, channel, 0, (uint8_t *) &st, sizeof(st), 1000);
    if (rc < LIBUSB_SUCCESS)
        return false;

    state->state = st.state;
    state->rxerr = st.rxerr;
    state->txerr = st.txerr;
    return true;
}

bool candle_send_frame_unsafe(struct candle_device *device, uint8_t channel, struct candle_can_frame *frame) {
    struct candle_device_handle *handle = device->handle;

    if (!handle->channels[channel].is_start)
        return false;

    if (frame->can_dlc > 15)
        return false;

    if (frame->type & CANDLE_FRAME_TYPE_FD && !(device->channels[channel].feature & CANDLE_FEATURE_FD))
        return false;

    struct gs_host_frame *hf;
    size_t hf_size_tx;
    if (frame->type & CANDLE_FRAME_TYPE_FD) {
        if (device->channels[channel].feature & CANDLE_FEATURE_REQ_USB_QUIRK_LPC546XX)
            hf_size_tx = struct_size(hf, canfd_quirk, 1);
        else
            hf_size_tx = struct_size(hf, canfd, 1);
    } else {
        if (device->channels[channel].feature & CANDLE_FEATURE_REQ_USB_QUIRK_LPC546XX)
            hf_size_tx = struct_size(hf, classic_can_quirk, 1);
        else
            hf_size_tx = struct_size(hf, classic_can, 1);
    }
    hf = malloc(hf_size_tx);
    if (hf == NULL)
        return false;

    hf->echo_id = frame->echo_id;

    hf->can_id = frame->can_id;
    if (frame->type & CANDLE_FRAME_TYPE_EFF)
        hf->can_id |= CAN_EFF_FLAG;
    if (frame->type & CANDLE_FRAME_TYPE_RTR)
        hf->can_id |= CAN_RTR_FLAG;
    if (frame->type & CANDLE_FRAME_TYPE_ERR)
        hf->can_id |= CAN_ERR_FLAG;

    hf->can_dlc = frame->can_dlc;
    hf->channel = channel;

    hf->flags = 0;
    if (frame->type & CANDLE_FRAME_TYPE_FD)
        hf->flags |= GS_CAN_FLAG_FD;
    if (frame->type & CANDLE_FRAME_TYPE_BRS)
        hf->flags |= GS_CAN_FLAG_BRS;
    if (frame->type & CANDLE_FRAME_TYPE_ESI)
        hf->flags |= GS_CAN_FLAG_ESI;

    hf->reserved = 0;

    size_t data_length = dlc2len[frame->can_dlc];

    if (frame->type & CANDLE_FRAME_TYPE_FD)
        memcpy(hf->canfd->data, frame->data, data_length);
    else
        memcpy(hf->classic_can->data, frame->data, data_length);

    struct libusb_transfer *transfer = libusb_alloc_transfer(0);
    if (transfer == NULL) {
        free(hf);
        return false;
    }

    libusb_fill_bulk_transfer(transfer, handle->usb_device_handle, handle->out_ep, (uint8_t *) hf, (int) hf_size_tx,
                              transmit_bulk_callback, handle, 1000);
    int rc = libusb_submit_transfer(transfer);
    if (rc != LIBUSB_SUCCESS) {
        free(hf);
        libusb_free_transfer(transfer);
    }

    return true;
}

bool candle_receive_frame_unsafe(struct candle_device *device, uint8_t channel, struct candle_can_frame *frame) {
    struct candle_device_handle *handle = device->handle;

    if (!handle->channels[channel].is_start)
        return false;

    struct gs_host_frame *hf = alloca(handle->rx_size);
    if (fifo_get(handle->channels[channel].rx_fifo, hf) < 0)
        return false;

    frame->type = 0;
    if (hf->echo_id == 0xFFFFFFFF)
        frame->type |= CANDLE_FRAME_TYPE_RX;
    if (hf->can_id & CAN_EFF_FLAG)
        frame->type |= CANDLE_FRAME_TYPE_EFF;
    if (hf->can_id & CAN_RTR_FLAG)
        frame->type |= CANDLE_FRAME_TYPE_RTR;
    if (hf->can_id & CAN_ERR_FLAG)
        frame->type |= CANDLE_FRAME_TYPE_ERR;
    if (hf->flags & GS_CAN_FLAG_FD)
        frame->type |= CANDLE_FRAME_TYPE_FD;
    if (hf->flags & GS_CAN_FLAG_BRS)
        frame->type |= CANDLE_FRAME_TYPE_BRS;
    if (hf->flags & GS_CAN_FLAG_ESI)
        frame->type |= CANDLE_FRAME_TYPE_ESI;

    frame->echo_id = hf->echo_id;

    if (hf->can_id & CAN_EFF_FLAG)
        frame->can_id = hf->can_id & 0x1FFFFFFF;
    else
        frame->can_id = hf->can_id & 0x7FF;

    frame->can_dlc = hf->can_dlc;

    if (hf->flags & GS_CAN_FLAG_FD) {
        memcpy(frame->data, hf->canfd->data, dlc2len[hf->can_dlc]);
        if (handle->channels[channel].mode & CANDLE_MODE_HW_TIMESTAMP)
            frame->timestamp_us = hf->canfd_ts->timestamp_us;
    } else {
        memcpy(frame->data, hf->classic_can->data, dlc2len[hf->can_dlc]);
        if (handle->channels[channel].mode & CANDLE_MODE_HW_TIMESTAMP)
            frame->timestamp_us = hf->classic_can_ts->timestamp_us;
    }

    return true;
}

bool candle_wait_frame_unsafe(struct candle_device *device, uint8_t channel, uint32_t milliseconds) {
    struct candle_device_handle *handle = device->handle;

    MUTEX_LOCK(handle->channels[channel].rx_fifo->mutex);
    bool empty = fifo_is_empty(handle->channels[channel].rx_fifo);
    if (empty)
        mtx_lock(&handle->channels[channel].rx_cond_mtx);
    MUTEX_UNLOCK(handle->channels[channel].rx_fifo->mutex);

    if (!empty)
        return true;

    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    ts.tv_sec += milliseconds / 1000;
    ts.tv_nsec += (milliseconds % 1000) * 1000000;

    if (ts.tv_nsec > 1000000) {
        ts.tv_nsec -= 1000000;
        ts.tv_sec += 1;
    }

    bool r = cnd_timedwait(&handle->channels[channel].rx_cnd, &handle->channels[channel].rx_cond_mtx, &ts) ==
             thrd_success;
    mtx_unlock(&handle->channels[channel].rx_cond_mtx);
    return r;
}

static mtx_t api_mtx;
static once_flag api_mtx_init = ONCE_FLAG_INIT;

static void init_api_mutex(void) {
    mtx_init(&api_mtx, mtx_plain);
}

static void api_lock(void) {
    call_once(&api_mtx_init, init_api_mutex);
    mtx_lock(&api_mtx);
}

static void api_unlock(void) {
    call_once(&api_mtx_init, init_api_mutex);
    mtx_unlock(&api_mtx);
}

bool candle_initialize(void) {
    bool r;
    api_lock();
    r = candle_initialize_unsafe();
    api_unlock();
    return r;
}

bool candle_finalize(void) {
    bool r;
    api_lock();
    r = candle_finalize_unsafe();
    api_unlock();
    return r;
}

bool candle_list_device(struct candle_device *devices[], size_t *size) {
    bool r;
    api_lock();
    r = candle_list_device_unsafe(devices, size);
    api_unlock();
    return r;
}

bool candle_open_device(struct candle_device *device) {
    bool r;
    api_lock();
    r = candle_open_device_unsafe(device);
    api_unlock();
    return r;
}

bool candle_close_device(struct candle_device *device) {
    bool r;
    api_lock();
    r = candle_close_device_unsafe(device);
    api_unlock();
    return r;
}

bool candle_reset_channel(struct candle_device *device, uint8_t channel) {
    bool r;
    api_lock();
    r = candle_reset_channel_unsafe(device, channel);
    api_unlock();
    return r;
}

bool candle_start_channel(struct candle_device *device, uint8_t channel, enum candle_mode mode) {
    bool r;
    api_lock();
    r = candle_start_channel_unsafe(device, channel, mode);
    api_unlock();
    return r;
}

bool candle_set_bit_timing(struct candle_device *device, uint8_t channel, struct candle_bit_timing *bit_timing) {
    bool r;
    api_lock();
    r = candle_set_bit_timing_unsafe(device, channel, bit_timing);
    api_unlock();
    return r;
}

bool candle_set_data_bit_timing(struct candle_device *device, uint8_t channel, struct candle_bit_timing *bit_timing) {
    bool r;
    api_lock();
    r = candle_set_data_bit_timing_unsafe(device, channel, bit_timing);
    api_unlock();
    return r;
}

bool candle_get_termination(struct candle_device *device, uint8_t channel, bool *enable) {
    bool r;
    api_lock();
    r = candle_get_termination_unsafe(device, channel, enable);
    api_unlock();
    return r;
}

bool candle_set_termination(struct candle_device *device, uint8_t channel, bool enable) {
    bool r;
    api_lock();
    r = candle_set_termination_unsafe(device, channel, enable);
    api_unlock();
    return r;
}

bool candle_get_state(struct candle_device *device, uint8_t channel, struct candle_state *state) {
    bool r;
    api_lock();
    r = candle_get_state_unsafe(device, channel, state);
    api_unlock();
    return r;
}

bool candle_send_frame(struct candle_device *device, uint8_t channel, struct candle_can_frame *frame) {
    bool r;
    api_lock();
    r = candle_send_frame_unsafe(device, channel, frame);
    api_unlock();
    return r;
}

bool candle_receive_frame(struct candle_device *device, uint8_t channel, struct candle_can_frame *frame) {
    bool r;
    api_lock();
    r = candle_receive_frame_unsafe(device, channel, frame);
    api_unlock();
    return r;
}

bool candle_wait_frame(struct candle_device *device, uint8_t channel, uint32_t milliseconds) {
    bool r;
    api_lock();
    r = candle_wait_frame_unsafe(device, channel, milliseconds);
    api_unlock();
    return r;
}
