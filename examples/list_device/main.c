#include "candle_api.h"
#include <stdio.h>


int main(int argc, char *argv[]) {
    // initialize library
    candle_initialize();

    // list device
    struct candle_device **device_list;
    size_t device_list_size;
    candle_get_device_list(&device_list, &device_list_size);

    // print device info
    for (size_t i = 0; i < device_list_size; ++i) {
        struct candle_device *dev = device_list[i];
        printf("%04X:%04X - %s - %s - %s\n", dev->vendor_id, dev->product_id, dev->manufacturer, dev->product, dev->serial_number);
    }

    // free device list
    candle_free_device_list(device_list);

    // finalize library
    candle_finalize();
    return 0;
}
