#include "candle_api.h"
#include "stdlib.h"
#include "stdio.h"


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
    success = candle_list_device(NULL, &device_list_size);
    if (!success)
        goto handle_error;
    if (device_list_size == 0) {
        printf("no device available\n");
        goto finalize;
    }

    // alloc memory for device list
    device_list = calloc(sizeof(struct candle_device *), device_list_size);
    if (device_list == NULL)
        goto handle_error;

    // list device again
    success = candle_list_device(device_list, &device_list_size);
    if (!success)
        goto handle_error;

    // double check list size
    if (device_list_size == 0) {
        printf("no device available\n");
        goto finalize;
    }

    // using first device
    struct candle_device *dev = device_list[0];

    // free device list
    free(device_list);

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
    struct candle_can_frame frame = {.type = 0, .echo_id = 123, .can_id = 0, .can_dlc = 8, .data = {1, 2, 3, 4, 5, 6, 7, 8}};
    success = candle_send_frame(dev, 0, &frame);
    if (!success)
        goto handle_error;
    printf("send frame (echo id: %d)\n", frame.echo_id);

    // receive message
    while (candle_wait_frame(dev, 0, 1000) && candle_receive_frame(dev, 0, &frame)) {
        printf("received frame (echo id: %d)\n", frame.echo_id);
    }

    // close device
    success = candle_close_device(dev);
    if (!success)
        goto handle_error;

    goto finalize;

handle_error:
    printf("error occur\n");

finalize:
    // finalize library
    candle_finalize();
    return 0;
}
