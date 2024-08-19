#include "candle_api.h"
#include "stdlib.h"
#include "stdio.h"


int main(int argc, char *argv[]) {
    // initialize library
    candle_initialize();

    // list device
    struct candle_device **device_list;
    size_t device_list_size = 0;
    candle_list_device(NULL, &device_list_size);
    if (device_list_size == 0) {
        printf("no device available\n");
        return 0;
    }

    // alloc memory for device list
    device_list = calloc(sizeof(struct candle_device *), device_list_size);
    if (device_list == NULL)
        return -1;

    // list device again
    candle_list_device(device_list, &device_list_size);

    // print device info
    for (size_t i = 0; i < device_list_size; ++i) {
        struct candle_device *dev = device_list[i];
        candle_open_device(dev);
        candle_start_channel(dev, 0, CANDLE_MODE_FD | CANDLE_MODE_HW_TIMESTAMP);
        printf("%04X:%04X - %s - %s - %s\n", dev->vendor_id, dev->product_id, dev->manufacturer, dev->product, dev->serial_number);
    }

    // free device list
    free(device_list);

    // finalize library
    candle_finalize();
    return 0;
}
