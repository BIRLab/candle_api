#include "candle_api.h"
#include <stdio.h>


int main(int argc, char *argv[]) {
    bool success;

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
    // candle_set_bit_timing(dev, 0, ...);
    // candle_set_data_bit_timing(dev, 0, ...);

    // set termination
    // candle_set_termination(dev, 0, true);

    // start channel 0
    success = candle_start_channel(dev, 0, CANDLE_MODE_LISTEN_ONLY | CANDLE_MODE_LOOP_BACK);
    if (!success)
        goto handle_error;

    // send message
    struct candle_can_frame frame = {.type = 0, .can_id = 0, .can_dlc = 8, .data = {1, 2, 3, 4, 5, 6, 7, 8}};
    success = candle_send_frame(dev, 0, &frame);
    if (!success)
        goto handle_error;
    printf("send frame (id: %d)\n", frame.can_id);

    // receive message
    while (candle_wait_and_receive_frame(dev, 0, &frame, 1000)) {
        printf("received frame (id: %d)\n", frame.can_id);
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
