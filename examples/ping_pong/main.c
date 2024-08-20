#include "candle_api.h"
#include "stdlib.h"
#include "stdio.h"
#include <signal.h>


static const size_t dlc2len[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};
static bool interrupt;


void signal_handle(int signal) {
    interrupt = true;
}


int main(int argc, char *argv[]) {
    bool success;

    // catch signal to exit
    signal(SIGINT, signal_handle);
    signal(SIGTERM, signal_handle);

    // initialize library
    success = candle_initialize();
    if (!success) {
        printf("initialize failure\n");
        return -1;
    }

    // list device
    struct candle_device **device_list;
    size_t device_list_size = 0;
    success = candle_get_device_list(&device_list, &device_list_size);
    if (!success)
        goto handle_error;
    if (device_list_size == 0) {
        printf("no device available\n");
        goto finalize;
    }

    // using first device
    struct candle_device *dev = device_list[0];
    candle_ref_device(dev);

    // free device list
    candle_free_device_list(device_list);

    // open device
    success = candle_open_device(dev);
    if (!success)
        goto handle_error;

    // set bit timing
    struct candle_bit_timing bt = {.prop_seg = 1, .phase_seg1 = 43, .phase_seg2 = 15, .sjw = 15, .brp = 2};
    success = candle_set_bit_timing(dev, 0, &bt);
    if (!success)
        goto handle_error;

    // set data bit timing
    if (dev->channels[0].feature & CANDLE_FEATURE_FD) {
        struct candle_bit_timing dbt = {.prop_seg = 1, .phase_seg1 = 4, .phase_seg2 = 2, .sjw = 2, .brp = 1};
        success = candle_set_data_bit_timing(dev, 0, &dbt);
        if (!success)
            goto handle_error;
    }

    // set termination
    if (dev->channels[0].feature & CANDLE_FEATURE_TERMINATION) {
        success = candle_set_termination(dev, 0, true);
        if (!success)
            goto handle_error;
    }

    // start channel 0
    enum candle_mode mode = CANDLE_MODE_NORMAL;
    if (dev->channels[0].feature & CANDLE_FEATURE_FD)
        mode |= CANDLE_MODE_FD;
    if (dev->channels[0].feature & CANDLE_FEATURE_HW_TIMESTAMP)
        mode |= CANDLE_MODE_HW_TIMESTAMP;
    success = candle_start_channel(dev, 0, mode);
    if (!success)
        goto handle_error;

    // receive message
    struct candle_can_frame frame;
    while (!interrupt) {
        if (!candle_wait_and_receive_frame(dev, 0, &frame, 1000))
            continue;

        if (!(frame.type & CANDLE_FRAME_TYPE_RX))
            continue;

        printf("received frame, id: 0x%X, data: ", frame.can_id);
        for (int i = 0; i < dlc2len[frame.can_dlc]; ++i) {
            if (i == 0)
                printf("%02X", frame.data[i]);
            else
                printf(" %02X", frame.data[i]);
        }
        printf("\n");
    }

    // close device
    candle_close_device(dev);
    candle_unref_device(dev);

    goto finalize;

handle_error:
    printf("error occur\n");

finalize:
    // finalize library
    candle_finalize();
    return 0;
}
