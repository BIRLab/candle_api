#include "candle_api.h"
#include "compiler.h"
#include "libusb.h"
#include "list.h"
#include "fifo.h"
#include "gs_usb_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

static const uint8_t dlc2len[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};
static struct libusb_context *ctx = NULL;
static LIST_HEAD(device_list);
static size_t open_device_count;
static thrd_t event_thread;
static bool event_thread_run;

struct candle_channel_handle {
    bool is_start;
    enum candle_mode mode;
    fifo_t *rx_fifo;
    cnd_t rx_cnd;
    mtx_t rx_cond_mtx;
    atomic_uint_fast32_t echo_id_pool;
    cnd_t echo_id_cnd;
    mtx_t echo_id_cond_mtx;
};

struct candle_device_handle {
    struct list_head list;
    struct candle_device *device;
    struct libusb_device *usb_device;
    struct libusb_device_handle *usb_device_handle;
    struct libusb_transfer *rx_transfer;
    size_t ref_count;
    size_t rx_size;
    uint8_t in_ep;
    uint8_t out_ep;
    struct candle_channel_handle channels[];
};

static int event_thread_func(void *arg) {
    while (event_thread_run) {
        libusb_handle_events(arg);
        thrd_yield();
    }
    thrd_exit(0);
}

static void after_libusb_open_hook(void) {
    open_device_count++;
    if (open_device_count == 1) {
        // start event loop
        event_thread_run = true;
        thrd_create(&event_thread, event_thread_func, ctx);
    }
}

static void before_libusb_close_hook(void) {
    open_device_count--;
    if (open_device_count == 0) {
        // stop event loop
        event_thread_run = false;
    }
}

static void after_libusb_close_hook(void) {
    if (open_device_count == 0) {
        // join event loop
        thrd_join(event_thread, NULL);
    }
}

static void LIBUSB_CALL receive_bulk_callback(struct libusb_transfer *transfer) {
    struct candle_device_handle *handle = transfer->user_data;
    struct gs_host_frame *hf = (struct gs_host_frame *)transfer->buffer;
    uint8_t ch = hf->channel;

    switch (transfer->status) {
        case LIBUSB_TRANSFER_COMPLETED:
            if (ch < handle->device->channel_count && handle->channels[ch].is_start) {
                // release echo id
                if (hf->echo_id != 0xFFFFFFFF) {
                    mtx_lock(&handle->channels[ch].echo_id_cond_mtx);
                    atomic_fetch_and(&handle->channels[ch].echo_id_pool, ~(1 << hf->echo_id));
                    cnd_signal(&handle->channels[ch].echo_id_cnd);
                    mtx_unlock(&handle->channels[ch].echo_id_cond_mtx);
                }

                // put in fifo
                fifo_put(handle->channels[ch].rx_fifo, hf);
                mtx_lock(&handle->channels[ch].rx_cond_mtx);
                cnd_signal(&handle->channels[ch].rx_cnd);
                mtx_unlock(&handle->channels[ch].rx_cond_mtx);
            }
            libusb_submit_transfer(transfer);
            break;
        case LIBUSB_TRANSFER_CANCELLED:
            free(transfer->buffer);
            libusb_free_transfer(transfer);
            handle->rx_transfer = NULL;
            break;
        case LIBUSB_TRANSFER_NO_DEVICE:
            handle->device->is_connected = false;
            free(transfer->buffer);
            libusb_free_transfer(transfer);
            handle->rx_transfer = NULL;
            break;
        default:
            libusb_submit_transfer(transfer);
    }
}

static void LIBUSB_CALL transmit_bulk_callback(struct libusb_transfer *transfer) {
    struct candle_device_handle *handle = transfer->user_data;

    switch (transfer->status) {
        case LIBUSB_TRANSFER_COMPLETED:
        case LIBUSB_TRANSFER_CANCELLED:
            free(transfer->buffer);
            libusb_free_transfer(transfer);
            break;
        case LIBUSB_TRANSFER_NO_DEVICE:
            free(transfer->buffer);
            libusb_free_transfer(transfer);
            handle->device->is_connected = false;
            break;
        default:
            libusb_submit_transfer(transfer);
    }
}

static void free_device(struct candle_device_handle* handle) {
    list_del(&handle->list);
    if (handle->rx_transfer != NULL) {
        libusb_cancel_transfer(handle->rx_transfer);
    }
    if (handle->usb_device_handle != NULL) {
        struct gs_device_mode md = {.mode = 0};
        for (int i = 0; i < handle->device->channel_count; ++i) {
            libusb_control_transfer(handle->usb_device_handle,
                                    LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                    GS_USB_BREQ_MODE, i, 0, (uint8_t *) &md, sizeof(md), 1000);
        }
        libusb_release_interface(handle->usb_device_handle, 0);
        before_libusb_close_hook();
        libusb_close(handle->usb_device_handle);
        after_libusb_close_hook();
    }
    for (int i = 0; i < handle->device->channel_count; ++i) {
        fifo_destroy(handle->channels[i].rx_fifo);
        cnd_destroy(&handle->channels[i].rx_cnd);
        mtx_destroy(&handle->channels[i].rx_cond_mtx);
        cnd_destroy(&handle->channels[i].echo_id_cnd);
        mtx_destroy(&handle->channels[i].echo_id_cond_mtx);
    }
    libusb_unref_device(handle->usb_device);
    free(handle->device);
    free(handle);
}

static void convert_frame(struct gs_host_frame *hf, struct candle_can_frame *frame, bool hw_timestamp) {
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

    if (hf->can_id & CAN_EFF_FLAG)
        frame->can_id = hf->can_id & 0x1FFFFFFF;
    else
        frame->can_id = hf->can_id & 0x7FF;

    frame->can_dlc = hf->can_dlc;

    if (hf->flags & GS_CAN_FLAG_FD) {
        memcpy(frame->data, hf->canfd->data, dlc2len[hf->can_dlc]);
        if (hw_timestamp)
            frame->timestamp_us = hf->canfd_ts->timestamp_us;
    } else {
        memcpy(frame->data, hf->classic_can->data, dlc2len[hf->can_dlc]);
        if (hw_timestamp)
            frame->timestamp_us = hf->classic_can_ts->timestamp_us;
    }
}

static bool send_frame(struct candle_device_handle* handle, uint8_t channel, struct candle_can_frame *frame, uint32_t echo_id) {
    // calculate tx size
    struct gs_host_frame *hf;
    size_t hf_size_tx;
    if (frame->type & CANDLE_FRAME_TYPE_FD) {
        if (handle->device->channels[channel].feature & CANDLE_FEATURE_REQ_USB_QUIRK_LPC546XX)
            hf_size_tx = struct_size(hf, canfd_quirk, 1);
        else
            hf_size_tx = struct_size(hf, canfd, 1);
    } else {
        if (handle->device->channels[channel].feature & CANDLE_FEATURE_REQ_USB_QUIRK_LPC546XX)
            hf_size_tx = struct_size(hf, classic_can_quirk, 1);
        else
            hf_size_tx = struct_size(hf, classic_can, 1);
    }

    // allocate frame buffer (buffer will be free in transmit_bulk_callback)
    hf = malloc(hf_size_tx);
    if (hf == NULL)
        return false;

    hf->echo_id = echo_id;

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

    // allocate transfer (transfer will be free in transmit_bulk_callback)
    struct libusb_transfer *transfer = libusb_alloc_transfer(0);
    if (transfer == NULL) {
        free(hf);
        return false;
    }

    // submit transfer
    libusb_fill_bulk_transfer(transfer, handle->usb_device_handle, handle->out_ep, (uint8_t *)hf, (int)hf_size_tx,
                              transmit_bulk_callback, handle, 1000);
    int rc = libusb_submit_transfer(transfer);
    if (rc != LIBUSB_SUCCESS) {
        if (rc == LIBUSB_ERROR_NO_DEVICE)
            handle->device->is_connected = false;
        free(hf);
        libusb_free_transfer(transfer);
    }

    return true;
}

static void milliseconds_to_timespec(uint32_t milliseconds, struct timespec *ts) {
    timespec_get(ts, TIME_UTC);
    ts->tv_sec += milliseconds / 1000;
    ts->tv_nsec += (long)(milliseconds % 1000) * 1000000;

    if (ts->tv_nsec > 1000000000) {
        ts->tv_nsec -= 1000000000;
        ts->tv_sec += 1;
    }
}

bool candle_initialize(void) {
    if (ctx == NULL && libusb_init_context(&ctx, NULL, 0) == LIBUSB_SUCCESS)
        return true;
    return false;
}

void candle_finalize(void) {
    if (ctx == NULL)
        return;

    struct candle_device_handle *pos;
    struct candle_device_handle *n;
    list_for_each_entry_safe(pos, n, &device_list, list) {
        free_device(pos);
    }
    libusb_exit(ctx);
    ctx = NULL;
}

bool candle_get_device_list(struct candle_device ***devices, size_t *size) {
    int rc;
    struct candle_device_handle *pos;
    struct candle_device_handle *n;

    // get usb device list
    struct libusb_device **usb_device_list;
    ssize_t count = libusb_get_device_list(ctx, &usb_device_list);
    if (count < 0) return false;

    // iterate usb device
    for (size_t i = 0; i < (size_t) count; ++i) {
        struct libusb_device *dev = usb_device_list[i];
        struct libusb_device_descriptor desc;

        // request usb descriptor
        rc = libusb_get_device_descriptor(dev, &desc);
        if (rc != LIBUSB_SUCCESS) continue;

        // check vid pid
        if ((desc.idVendor == 0x1d50 && desc.idProduct == 0x606f) ||
            (desc.idVendor == 0x1209 && desc.idProduct == 0x2323) ||
            (desc.idVendor == 0x1cd2 && desc.idProduct == 0x606f) ||
            (desc.idVendor == 0x16d0 && desc.idProduct == 0x10b8) ||
            (desc.idVendor == 0x16d0 && desc.idProduct == 0x0f30)) {

            // if a candle device already in device list
            bool old_device = false;
            list_for_each_entry(pos, &device_list, list) {
                if (pos->usb_device == dev) {
                    pos->ref_count++;   // ref once
                    old_device = true;
                }
            }

            // create new candle device
            if (!old_device) {
                // open usb device to request necessary information
                struct libusb_device_handle *dev_handle;
                rc = libusb_open(dev, &dev_handle);

                // cannot open device
                if (rc != LIBUSB_SUCCESS) {
                    if (rc == LIBUSB_ERROR_ACCESS) {
                        fprintf(stderr, "Found candle usb device %04x:%04x, but access denied. Please check permissions.\n", desc.idVendor, desc.idProduct);
                    }
                    continue;
                };

                // libusb open close hook
                after_libusb_open_hook();

                // read usb descriptions
                struct candle_device candle_dev;
                candle_dev.is_connected = true;
                candle_dev.is_open = false;
                candle_dev.vendor_id = desc.idVendor;
                candle_dev.product_id = desc.idProduct;
                rc = libusb_get_string_descriptor_ascii(dev_handle, desc.iManufacturer, (uint8_t *) candle_dev.manufacturer, sizeof(candle_dev.manufacturer));
                if (rc < 0)
                    memset(candle_dev.manufacturer, 0, sizeof(candle_dev.manufacturer));
                rc = libusb_get_string_descriptor_ascii(dev_handle, desc.iProduct, (uint8_t *) candle_dev.product, sizeof(candle_dev.product));
                if (rc < 0)
                    memset(candle_dev.manufacturer, 0, sizeof(candle_dev.product));
                rc = libusb_get_string_descriptor_ascii(dev_handle, desc.iSerialNumber, (uint8_t *) candle_dev.serial_number, sizeof(candle_dev.serial_number));
                if (rc < 0)
                    memset(candle_dev.manufacturer, 0, sizeof(candle_dev.serial_number));

                // send host config
                struct gs_host_config hconf;
                hconf.byte_order = 0x0000beef;
                rc = libusb_control_transfer(dev_handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, GS_USB_BREQ_HOST_FORMAT, 1, 0, (uint8_t *) &hconf, sizeof(hconf), 1000);
                if (rc < LIBUSB_SUCCESS) goto handle_error;

                // read device config
                struct gs_device_config dconf;
                rc = libusb_control_transfer(dev_handle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, GS_USB_BREQ_DEVICE_CONFIG, 1, 0, (uint8_t *) &dconf, sizeof(dconf), 1000);
                if (rc < LIBUSB_SUCCESS) goto handle_error;

                // channel count
                uint8_t channel_count = dconf.icount + 1;

                // update device information
                candle_dev.channel_count = channel_count;
                candle_dev.software_version = dconf.sw_version;
                candle_dev.hardware_version = dconf.hw_version;

                // alloc device memory
                struct candle_device *new_candle_device = malloc(sizeof(struct candle_device) + channel_count * sizeof(struct candle_channel));
                if (new_candle_device == NULL) goto handle_error;
                memcpy(new_candle_device, &candle_dev, sizeof(candle_dev));

                // request channel information
                for (uint8_t j = 0; j < channel_count; ++j) {
                    struct gs_device_bt_const bt_const;
                    rc = libusb_control_transfer(dev_handle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, GS_USB_BREQ_BT_CONST, j, 0, (uint8_t *) &bt_const, sizeof(bt_const), 1000);
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
                        new_candle_device->channels[j].feature |= CANDLE_FEATURE_REQ_USB_QUIRK_LPC546XX | CANDLE_FEATURE_QUIRK_BREQ_CANTACT_PRO;

                    if (!(dconf.sw_version > 1 && new_candle_device->channels[j].feature & CANDLE_FEATURE_IDENTIFY))
                        new_candle_device->channels[j].feature &= ~CANDLE_FEATURE_IDENTIFY;

                    if (bt_const.feature & CANDLE_FEATURE_FD && bt_const.feature & CANDLE_FEATURE_BT_CONST_EXT) {
                        struct gs_device_bt_const_extended bt_const_ext;
                        rc = libusb_control_transfer(dev_handle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, GS_USB_BREQ_BT_CONST_EXT, j, 0, (uint8_t *) &bt_const_ext, sizeof(bt_const_ext), 1000);
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
                        memset(&new_candle_device->channels[j].bit_timing_const.data, 0, sizeof(struct candle_bit_timing_const));
                }

                // find bulk endpoints
                uint8_t in_ep = 0;
                uint8_t out_ep = 0;
                struct libusb_config_descriptor *conf_desc;
                rc = libusb_get_active_config_descriptor(dev, &conf_desc);
                if (rc != LIBUSB_SUCCESS) {
                    free(new_candle_device);
                    goto handle_error;
                }
                for (uint8_t j = 0; j < conf_desc->interface[0].altsetting[0].bNumEndpoints; ++j) {
                    uint8_t ep_address = conf_desc->interface[0].altsetting[0].endpoint[j].bEndpointAddress;
                    uint8_t ep_type = conf_desc->interface[0].altsetting[0].endpoint[j].bmAttributes;
                    if (ep_address & LIBUSB_ENDPOINT_IN && ep_type & LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK)
                        in_ep = ep_address;
                    if (!(ep_address & LIBUSB_ENDPOINT_IN) && ep_type & LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK)
                        out_ep = ep_address;
                }
                libusb_free_config_descriptor(conf_desc);
                if (!(in_ep && out_ep)) {
                    free(new_candle_device);
                    goto handle_error;
                }

                // calculate rx size
                struct gs_host_frame *hf;
                size_t rx_hf_size;
                size_t rx_size = 0;
                for (int j = 0; j < channel_count; ++j) {
                    if (new_candle_device->channels[j].feature & CANDLE_FEATURE_FD) {
                        if (new_candle_device->channels[j].feature & CANDLE_FEATURE_HW_TIMESTAMP)
                            rx_hf_size = struct_size(hf, canfd_ts, 1);
                        else
                            rx_hf_size = struct_size(hf, canfd, 1);
                    } else {
                        if (new_candle_device->channels[j].feature & CANDLE_FEATURE_HW_TIMESTAMP)
                            rx_hf_size = struct_size(hf, classic_can_ts, 1);
                        else
                            rx_hf_size = struct_size(hf, classic_can, 1);
                    }
                    rx_size = max(rx_size, rx_hf_size);
                }

                // create internal handle
                struct candle_device_handle *handle = malloc(sizeof(struct candle_device_handle) + channel_count * sizeof(struct candle_channel_handle));
                if (handle == NULL) {
                    free(new_candle_device);
                    goto handle_error;
                }
                handle->device = new_candle_device;
                handle->usb_device = libusb_ref_device(dev);
                handle->usb_device_handle = NULL;
                handle->rx_transfer = NULL;
                handle->ref_count = 1;  // ref once
                handle->rx_size = rx_size;
                handle->in_ep = in_ep;
                handle->out_ep = out_ep;

                // create internal channel handle
                for (int j = 0; j < channel_count; ++j) {
                    handle->channels[j].is_start = false;
                    handle->channels[j].mode = CANDLE_MODE_NORMAL;
                    handle->channels[j].rx_fifo = fifo_create((char)rx_size, 1024); // no more than 81920 bytes plus sizeof(fifo_t)
                    cnd_init(&handle->channels[j].rx_cnd);
                    mtx_init(&handle->channels[j].rx_cond_mtx, mtx_plain);
                    cnd_init(&handle->channels[j].echo_id_cnd);
                    mtx_init(&handle->channels[j].echo_id_cond_mtx, mtx_plain);
                    atomic_init(&handle->channels[j].echo_id_pool, 0);
                }

                // set candle device handle
                new_candle_device->handle = handle;

                // add handle to device list
                list_add_tail(&handle->list, &device_list);

handle_error:
                // libusb open close hook
                before_libusb_close_hook();

                libusb_close(dev_handle);

                // libusb open close hook
                after_libusb_close_hook();
            }
        }
    }

    // free usb device list
    libusb_free_device_list(usb_device_list, 1);

    // calculate list size
    *size = list_count_nodes(&device_list);

    // prepare output device list (NULL terminated)
    struct candle_device **output_device_list = calloc(*size + 1, sizeof(struct candle_device *));

    // cannot alloc memory
    if (output_device_list == NULL) {
        // unref once
        list_for_each_entry_safe(pos, n, &device_list, list) {
            pos->ref_count--;
            if (pos->ref_count == 0)
                free_device(pos);
        }
        return false;
    }

    // fill output device list
    size_t i = 0;
    list_for_each_entry(pos, &device_list, list) {
        output_device_list[i] = pos->device;
        i++;
    }

    // success
    *devices = output_device_list;
    return true;
}

void candle_free_device_list(struct candle_device **devices) {
    // unref device
    for(int i = 0; devices[i] != NULL; ++i) {
        devices[i]->handle->ref_count--;
        if (devices[i]->handle->ref_count == 0)
            free_device(devices[i]->handle);
    }

    // free list
    free(devices);
}

struct candle_device *candle_ref_device(struct candle_device *device) {
    device->handle->ref_count++;
    return device;
}

void candle_unref_device(struct candle_device *device) {
    device->handle->ref_count--;
    if (device->handle->ref_count == 0)
        free_device(device->handle);
}

bool candle_open_device(struct candle_device *device) {
    struct candle_device_handle *handle = device->handle;

    // avoid double open
    if (device->is_open)
        return false;

    // open usb device
    int rc = libusb_open(handle->usb_device, &handle->usb_device_handle);
    if (rc != LIBUSB_SUCCESS) {
        if (rc == LIBUSB_ERROR_NO_DEVICE)
            device->is_connected = false;
        handle->usb_device_handle = NULL;
        return false;
    }

    // libusb open close hook
    after_libusb_open_hook();

    // detach kernel driver
    rc = libusb_set_auto_detach_kernel_driver(handle->usb_device_handle, 1);
    if (rc != LIBUSB_SUCCESS && rc != LIBUSB_ERROR_NOT_SUPPORTED) {
        before_libusb_close_hook();
        libusb_close(handle->usb_device_handle);
        after_libusb_close_hook();
        handle->usb_device_handle = NULL;
        return false;
    }

    // claim interface
    rc = libusb_claim_interface(handle->usb_device_handle, 0);
    if (rc != LIBUSB_SUCCESS) {
        if (rc == LIBUSB_ERROR_NO_DEVICE)
            device->is_connected = false;
        before_libusb_close_hook();
        libusb_close(handle->usb_device_handle);
        after_libusb_close_hook();
        handle->usb_device_handle = NULL;
        return false;
    }

    // reset channel
    struct gs_device_mode md = {.mode = 0};
    for (int i = 0; i < device->channel_count; ++i) {
        rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_MODE, i, 0, (uint8_t *) &md, sizeof(md), 1000);
        if (rc < LIBUSB_SUCCESS) {
            if (rc == LIBUSB_ERROR_NO_DEVICE)
                device->is_connected = false;
            libusb_release_interface(handle->usb_device_handle, 0);
            before_libusb_close_hook();
            libusb_close(handle->usb_device_handle);
            after_libusb_close_hook();
            handle->usb_device_handle = NULL;
            return false;
        }
    }

    // alloc transfer
    handle->rx_transfer = libusb_alloc_transfer(0);
    if (handle->rx_transfer == NULL) {
        libusb_release_interface(handle->usb_device_handle, 0);
        before_libusb_close_hook();
        libusb_close(handle->usb_device_handle);
        after_libusb_close_hook();
        handle->usb_device_handle = NULL;
        return false;
    }

    // alloc frame buffer
    uint8_t *fb = malloc(handle->rx_size);
    if (fb == NULL) {
        libusb_free_transfer(handle->rx_transfer);
        handle->rx_transfer = NULL;
        libusb_release_interface(handle->usb_device_handle, 0);
        before_libusb_close_hook();
        libusb_close(handle->usb_device_handle);
        after_libusb_close_hook();
        handle->usb_device_handle = NULL;
        return false;
    }

    // fill transfer
    libusb_fill_bulk_transfer(handle->rx_transfer, handle->usb_device_handle, handle->in_ep,
                              fb, (int) handle->rx_size, receive_bulk_callback, handle, 1000);

    // submit transfer
    rc = libusb_submit_transfer(handle->rx_transfer);
    if (rc != LIBUSB_SUCCESS) {
        if (rc == LIBUSB_ERROR_NO_DEVICE)
            device->is_connected = false;
        free(fb);
        libusb_free_transfer(handle->rx_transfer);
        handle->rx_transfer = NULL;
        libusb_release_interface(handle->usb_device_handle, 0);
        before_libusb_close_hook();
        libusb_close(handle->usb_device_handle);
        after_libusb_close_hook();
        handle->usb_device_handle = NULL;
        return false;
    }

    // increase ref count
    handle->ref_count++;

    // success
    device->is_open = true;
    return true;
}

void candle_close_device(struct candle_device *device) {
    struct candle_device_handle *handle = device->handle;

    // avoid double close
    if (!device->is_open)
        return;

    // cancel transfer (rx_transfer and buffer will be free in receive_bulk_callback)
    if (handle->rx_transfer != NULL) {
        libusb_cancel_transfer(handle->rx_transfer);
        handle->rx_transfer = NULL;
    }

    // reset channel (best efforts)
    int rc;
    struct gs_device_mode md = {.mode = 0};
    for (int i = 0; i < device->channel_count; ++i) {
        rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_MODE, i, 0, (uint8_t *) &md, sizeof(md), 1000);
        if (rc == LIBUSB_ERROR_NO_DEVICE)
            device->is_connected = false;
        fifo_flush(handle->channels[i].rx_fifo);
        atomic_store(&handle->channels[i].echo_id_pool, 0);
        handle->channels[i].mode = CANDLE_MODE_NORMAL;
        handle->channels[i].is_start = false;
    }

    // release interface
    rc = libusb_release_interface(handle->usb_device_handle, 0);
    if (rc == LIBUSB_ERROR_NO_DEVICE)
        device->is_connected = false;

    // close usb device
    before_libusb_close_hook();
    libusb_close(handle->usb_device_handle);
    after_libusb_close_hook();
    handle->usb_device_handle = NULL;

    // device is closed
    device->is_open = false;

    // decrease ref count
    handle->ref_count--;
    if (handle->ref_count == 0)
        free_device(handle);
}

bool candle_reset_channel(struct candle_device *device, uint8_t channel) {
    struct candle_device_handle *handle = device->handle;

    if (channel >= device->channel_count)
        return false;

    if (!device->is_open)
        return false;

    struct gs_device_mode md = {.mode = 0};
    int rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_MODE, channel, 0, (uint8_t *) &md, sizeof(md), 1000);
    if (rc < LIBUSB_SUCCESS) {
        if (rc == LIBUSB_ERROR_NO_DEVICE)
            device->is_connected = false;
        return false;
    }

    fifo_flush(handle->channels[channel].rx_fifo);
    atomic_store(&handle->channels[channel].echo_id_pool, 0);
    handle->channels[channel].mode = CANDLE_MODE_NORMAL;
    handle->channels[channel].is_start = false;

    return true;
}

bool candle_start_channel(struct candle_device *device, uint8_t channel, enum candle_mode mode) {
    struct candle_device_handle *handle = device->handle;

    if (channel >= device->channel_count)
        return false;

    if (!device->is_open)
        return false;

    struct gs_device_mode md = {.mode = 1, .flags = mode};
    int rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_MODE, channel, 0, (uint8_t *) &md, sizeof(md), 1000);
    if (rc < LIBUSB_SUCCESS) {
        if (rc == LIBUSB_ERROR_NO_DEVICE)
            device->is_connected = false;
        return false;
    }

    handle->channels[channel].mode = mode;
    handle->channels[channel].is_start = true;

    return true;
}

bool candle_set_bit_timing(struct candle_device *device, uint8_t channel, struct candle_bit_timing *bit_timing) {
    struct candle_device_handle *handle = device->handle;

    if (channel >= device->channel_count)
        return false;

    if (!device->is_open)
        return false;

    struct gs_device_bittiming bt = {.prop_seg = bit_timing->prop_seg, .phase_seg1 = bit_timing->phase_seg1, .phase_seg2 = bit_timing->phase_seg2, .sjw = bit_timing->sjw, .brp = bit_timing->brp};
    int rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_BITTIMING, channel, 0, (uint8_t *) &bt, sizeof(bt), 1000);
    if (rc < LIBUSB_SUCCESS) {
        if (rc == LIBUSB_ERROR_NO_DEVICE)
            device->is_connected = false;
        return false;
    }

    return true;
}

bool candle_set_data_bit_timing(struct candle_device *device, uint8_t channel, struct candle_bit_timing *bit_timing) {
    struct candle_device_handle *handle = device->handle;

    if (channel >= device->channel_count)
        return false;

    if (!device->is_open)
        return false;

    struct gs_device_bittiming bt = {.prop_seg = bit_timing->prop_seg, .phase_seg1 = bit_timing->phase_seg1, .phase_seg2 = bit_timing->phase_seg2, .sjw = bit_timing->sjw, .brp = bit_timing->brp};
    int rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_DATA_BITTIMING, channel, 0, (uint8_t *) &bt, sizeof(bt), 1000);
    if (rc < LIBUSB_SUCCESS) {
        if (rc == LIBUSB_ERROR_NO_DEVICE)
            device->is_connected = false;
        return false;
    }

    return true;
}

bool candle_get_termination(struct candle_device *device, uint8_t channel, bool *enable) {
    struct candle_device_handle *handle = device->handle;

    if (channel >= device->channel_count)
        return false;

    if (!device->is_open)
        return false;

    uint32_t state;
    int rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_GET_TERMINATION, channel, 0, (uint8_t *) &state, sizeof(state), 1000);
    if (rc < LIBUSB_SUCCESS) {
        if (rc == LIBUSB_ERROR_NO_DEVICE)
            device->is_connected = false;
        return false;
    }

    if (state)
        *enable = true;
    else
        *enable = false;
    return true;
}

bool candle_set_termination(struct candle_device *device, uint8_t channel, bool enable) {
    struct candle_device_handle *handle = device->handle;

    if (channel >= device->channel_count)
        return false;

    if (!device->is_open)
        return false;

    uint32_t state = enable ? 1 : 0;
    int rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_SET_TERMINATION, channel, 0, (uint8_t *) &state, sizeof(state), 1000);
    if (rc < LIBUSB_SUCCESS) {
        if (rc == LIBUSB_ERROR_NO_DEVICE)
            device->is_connected = false;
        return false;
    }

    return true;
}

bool candle_get_state(struct candle_device *device, uint8_t channel, struct candle_state *state) {
    struct candle_device_handle *handle = device->handle;

    if (channel >= device->channel_count)
        return false;

    if (!device->is_open)
        return false;

    struct gs_device_state st;
    int rc = libusb_control_transfer(handle->usb_device_handle,
                                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     GS_USB_BREQ_GET_STATE, channel, 0, (uint8_t *) &st, sizeof(st), 1000);
    if (rc < LIBUSB_SUCCESS) {
        if (rc == LIBUSB_ERROR_NO_DEVICE)
            device->is_connected = false;
        return false;
    }

    state->state = st.state;
    state->rxerr = st.rxerr;
    state->txerr = st.txerr;
    return true;
}

bool candle_send_frame_nowait(struct candle_device *device, uint8_t channel, struct candle_can_frame *frame) {
    struct candle_device_handle *handle = device->handle;

    if (channel >= device->channel_count)
        return false;

    if (!handle->channels[channel].is_start)
        return false;

    if (frame->can_dlc >= ARRAY_SIZE(dlc2len))
        return false;

    if (frame->type & CANDLE_FRAME_TYPE_FD && !(device->channels[channel].feature & CANDLE_FEATURE_FD))
        return false;

    // get echo id
    uint32_t echo_id_pool;
    uint32_t echo_id = 0;
    while (true) {
        // trying to preempt the echo id
        echo_id_pool = atomic_fetch_or(&handle->channels[channel].echo_id_pool, 1 << echo_id);

        // preempt the echo id
        if (!(echo_id_pool & (1 << echo_id))) {
            break;
        }

        // no echo id available
        if (echo_id_pool == (uint32_t)(-1)) {
            return false;
        }

        // find available echo id
        for (int i = 0; i < 32; ++i) {
            if (!(echo_id_pool & (1 << i))) {
                echo_id = i;
                break;
            }
        }
    }

    return send_frame(handle, channel, frame, echo_id);
}

bool candle_send_frame(struct candle_device *device, uint8_t channel, struct candle_can_frame *frame, uint32_t milliseconds) {
    struct candle_device_handle *handle = device->handle;

    struct timespec ts;
    milliseconds_to_timespec(milliseconds, &ts);

    if (channel >= device->channel_count)
        return false;

    if (!handle->channels[channel].is_start)
        return false;

    if (frame->can_dlc >= ARRAY_SIZE(dlc2len))
        return false;

    if (frame->type & CANDLE_FRAME_TYPE_FD && !(device->channels[channel].feature & CANDLE_FEATURE_FD))
        return false;

    // get echo id
    uint32_t echo_id_pool;
    uint32_t echo_id = 0;
    mtx_lock(&handle->channels[channel].echo_id_cond_mtx);
    while (true) {
        // trying to preempt the echo id
        echo_id_pool = atomic_fetch_or(&handle->channels[channel].echo_id_pool, 1 << echo_id);

        // preempt the echo id
        if (!(echo_id_pool & (1 << echo_id))) {
            mtx_unlock(&handle->channels[channel].echo_id_cond_mtx);
            break;
        }

        // no echo id available
        while (echo_id_pool == (uint32_t)(-1)) {
            if (cnd_timedwait(&handle->channels[channel].echo_id_cnd, &handle->channels[channel].echo_id_cond_mtx, &ts) == thrd_success) {
                echo_id_pool = atomic_load(&handle->channels[channel].echo_id_pool);
            }
            else {
                mtx_unlock(&handle->channels[channel].echo_id_cond_mtx);
                return false;
            }
        }

        // find available echo id
        for (int i = 0; i < 32; ++i) {
            if (!(echo_id_pool & (1 << i))) {
                echo_id = i;
                break;
            }
        }
    }

    return send_frame(handle, channel, frame, echo_id);
}

bool candle_receive_frame_nowait(struct candle_device *device, uint8_t channel, struct candle_can_frame *frame) {
    struct candle_device_handle *handle = device->handle;

    if (channel >= device->channel_count)
        return false;

    if (!handle->channels[channel].is_start)
        return false;

    struct gs_host_frame *hf = alloca(handle->rx_size);

    if (fifo_get(handle->channels[channel].rx_fifo, hf) < 0)
        return false;

    convert_frame(hf, frame, handle->channels[channel].mode & CANDLE_MODE_HW_TIMESTAMP);

    return true;
}

bool candle_receive_frame(struct candle_device *device, uint8_t channel, struct candle_can_frame *frame, uint32_t milliseconds) {
    struct candle_device_handle *handle = device->handle;

    if (channel >= device->channel_count)
        return false;

    if (!handle->channels[channel].is_start)
        return false;

    struct gs_host_frame *hf = alloca(handle->rx_size);

    MUTEX_LOCK(handle->channels[channel].rx_fifo->mutex);
    bool empty = fifo_get_noprotect(handle->channels[channel].rx_fifo, hf) < 0;
    if (empty)
        mtx_lock(&handle->channels[channel].rx_cond_mtx);
    MUTEX_UNLOCK(handle->channels[channel].rx_fifo->mutex);

    if (!empty) {
        convert_frame(hf, frame, handle->channels[channel].mode & CANDLE_MODE_HW_TIMESTAMP);
        return true;
    }

    struct timespec ts;
    milliseconds_to_timespec(milliseconds, &ts);

    bool r = cnd_timedwait(&handle->channels[channel].rx_cnd, &handle->channels[channel].rx_cond_mtx, &ts) == thrd_success;
    if (r) r = fifo_get(handle->channels[channel].rx_fifo, hf) == 0;
    mtx_unlock(&handle->channels[channel].rx_cond_mtx);

    if (r) convert_frame(hf, frame, handle->channels[channel].mode & CANDLE_MODE_HW_TIMESTAMP);
    return r;
}
