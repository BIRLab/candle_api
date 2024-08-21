#include "candle_api.h"
#include <stdio.h>
#include <time.h>
#include <threads.h>
#include <unistd.h>


static thrd_t receive_thread;
static bool interrupt;


struct test_epoch {
    int tx_cnt;
    int rx_cnt;
    int err_cnt;
    double dt;
    struct candle_device *dev;
};


static int receive_thread_func(void *arg) {
    struct test_epoch *e = arg;

    struct candle_can_frame frame;
    time_t st = time(NULL);
    while (!interrupt) {
        if (!candle_wait_and_receive_frame(e->dev, 0, &frame, 1000))
            continue;

        if (frame.type & CANDLE_FRAME_TYPE_ERR)
            e->err_cnt++;
        else {
            if (frame.type & CANDLE_FRAME_TYPE_RX)
                e->rx_cnt++;
            else
                e->tx_cnt++;
        }
    }
    e->dt = difftime(time(NULL), st);

    while (candle_wait_and_receive_frame(e->dev, 0, &frame, 1000));

    return 0;
}


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
        candle_free_device_list(device_list);
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

    // start channel 0 in loop back mode
    success = candle_start_channel(dev, 0, CANDLE_MODE_LISTEN_ONLY | CANDLE_MODE_LOOP_BACK);
    if (!success)
        goto handle_error;

    // start stress test
    struct test_epoch e;
    for (int i = 0; i < 3 && dev->is_connected; ++i) {
        printf("Test epoch %d\n", i + 1);

        // reset epoch
        e.tx_cnt = 0;
        e.rx_cnt = 0;
        e.err_cnt = 0;
        e.dev = dev;

        // start receive thread
        interrupt = false;
        thrd_create(&receive_thread, receive_thread_func, &e);

        // send message
        struct candle_can_frame frame = {.type = 0, .can_id = 0, .can_dlc = 8, .data = {1, 2, 3, 4, 5, 6, 7, 8}};
        for (int j = 0; j < 200000; ++j) {
            while ((dev->is_connected) && !candle_send_frame(dev, 0, &frame)) {
                candle_wait_frame(dev, 0, 1000);
                usleep(1000);
            }
        }

        // interrupt
        interrupt = true;

        // join thread
        thrd_join(receive_thread, NULL);

        // calculate result
        printf("tx: %d, rx %d, err: %d, dt: %d s\n", e.tx_cnt, e.rx_cnt, e.err_cnt, (int)e.dt);
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
