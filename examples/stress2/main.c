#include "candle_api.h"
#include <stdio.h>
#include <time.h>
#include <threads.h>


static thrd_t receive_thread1;
static thrd_t receive_thread2;
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
        if (!candle_receive_frame(e->dev, 0, &frame, 1000))
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

    while (candle_receive_frame(e->dev, 0, &frame, 1000));

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
    if (device_list_size < 2) {
        candle_free_device_list(device_list);
        printf("no device available\n");
        goto finalize;
    }

    // using two device
    struct candle_device *dev1 = device_list[0];
    struct candle_device *dev2 = device_list[1];
    candle_ref_device(dev1);
    candle_ref_device(dev2);

    // free device list
    candle_free_device_list(device_list);

    // open device
    success = candle_open_device(dev1);
    if (!success)
        goto handle_error;
    success = candle_open_device(dev2);
    if (!success)
        goto handle_error;

    // start channel 0
    success = candle_start_channel(dev1, 0, CANDLE_MODE_NORMAL);
    if (!success)
        goto handle_error;
    success = candle_start_channel(dev2, 0, CANDLE_MODE_NORMAL);
    if (!success)
        goto handle_error;

    // set bit timing (should set values according to the clock frequency)
    struct candle_bit_timing bt = {
        .prop_seg = 1,
        .phase_seg1 = 43,
        .phase_seg2 = 15,
        .sjw = 15,
        .brp = 2
    };
    success = candle_set_bit_timing(dev1, 0, &bt);
    if (!success)
        goto handle_error;
    success = candle_set_bit_timing(dev2, 0, &bt);
    if (!success)
        goto handle_error;

    // set termination
    if (dev1->channels[0].feature & CANDLE_FEATURE_TERMINATION) {
        success = candle_set_termination(dev1, 0, true);
        if (!success)
            goto handle_error;
    }
    if (dev2->channels[0].feature & CANDLE_FEATURE_TERMINATION) {
        success = candle_set_termination(dev2, 0, true);
        if (!success)
            goto handle_error;
    }

    // start stress test
    struct test_epoch e1;
    struct test_epoch e2;
    for (int i = 0; i < 3 && dev1->is_connected && dev2->is_connected; ++i) {
        printf("Test epoch %d\n", i + 1);

        // reset epoch
        e1.tx_cnt = 0;
        e1.rx_cnt = 0;
        e1.err_cnt = 0;
        e1.dev = dev1;

        e2.tx_cnt = 0;
        e2.rx_cnt = 0;
        e2.err_cnt = 0;
        e2.dev = dev2;

        // start receive thread
        interrupt = false;
        thrd_create(&receive_thread1, receive_thread_func, &e1);
        thrd_create(&receive_thread2, receive_thread_func, &e2);

        // send message
        struct candle_can_frame frame1 = {.type = 0, .can_id = 1, .can_dlc = 8, .data = {1, 2, 3, 4, 5, 6, 7, 8}};
        struct candle_can_frame frame2 = {.type = 0, .can_id = 2, .can_dlc = 8, .data = {1, 2, 3, 4, 5, 6, 7, 8}};
        for (int j = 0; j < 200000; ++j) {
            while ((dev1->is_connected) && !candle_send_frame(dev1, 0, &frame1, 1000));
            while ((dev2->is_connected) && !candle_send_frame(dev2, 0, &frame2, 1000));
        }

        // interrupt
        interrupt = true;

        // join thread
        thrd_join(receive_thread1, NULL);
        thrd_join(receive_thread2, NULL);

        // calculate result
        printf("dev: 1, tx: %d, rx %d, err: %d, dt: %d s\n", e1.tx_cnt, e1.rx_cnt, e1.err_cnt, (int)e1.dt);
        printf("dev: 2, tx: %d, rx %d, err: %d, dt: %d s\n", e2.tx_cnt, e2.rx_cnt, e2.err_cnt, (int)e2.dt);
    }

    // close device
    candle_close_device(dev1);
    candle_unref_device(dev2);

    goto finalize;

handle_error:
    printf("error occur\n");

finalize:
    // finalize library
    candle_finalize();
    return 0;
}
