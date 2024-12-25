const addon = require('bindings')('candle-js');

class CandleDevice {
    constructor(device) {
        this.vendor_id = device.vendor_id;
        this.product_id = device.product_id;
        this.manufacturer = device.manufacturer;
        this.product = device.product;
        this.serial_number = device.serial_number;
        this.handle = device.handle;
    }

    isOpened() {
        return addon.isOpened({'handle': this.handle});
    }

    open(mode) {
        addon.openDevice({'handle': this.handle});
        addon.resetChannel({'handle': this.handle}, 0);

        if ('termination' in mode)
            addon.setTermination({'handle': this.handle}, 0, mode['termination']);

        if ('bit_timing' in mode)
            addon.setBitTiming({'handle': this.handle}, 0, mode['bit_timing']);

        if ('data_bit_timing' in mode)
            addon.setDataBitTiming({'handle': this.handle}, 0, mode['data_bit_timing']);

        if ('fd' in mode && mode['fd'])
            addon.startChannel({'handle': this.handle}, 0, 1 << 8);
        else
            addon.startChannel({'handle': this.handle}, 0, 0);
    }

    close() {
        addon.closeDevice({'handle': this.handle});
    }

    send(frame, timeout = 1000) {
        return addon.send({'handle': this.handle}, 0, timeout, frame);
    }

    receive(timeout = 1000) {
        return addon.receive({'handle': this.handle}, 0, timeout);
    }
}

const wrapper = {
    listDevice: function listDevice() {
        list = [];
        for (const device of addon.listDevice()) {
            list.push(new CandleDevice(device));
        }
        return list;
    },

    FRAME_TYPE_RX: 1 << 0,
    FRAME_TYPE_EFF: 1 << 1,
    FRAME_TYPE_RTR: 1 << 2,
    FRAME_TYPE_ERR: 1 << 3,
    FRAME_TYPE_FD: 1 << 4,
    FRAME_TYPE_BRS: 1 << 5,
    FRAME_TYPE_ESI: 1 << 6
}

module.exports = wrapper;