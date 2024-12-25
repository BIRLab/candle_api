const addon = require('bindings')('candle-js');
const { once, EventEmitter } = require('events');

class CandleDevice extends EventEmitter {
    constructor(device) {
        super();
        this.vendor_id = device.vendor_id;
        this.product_id = device.product_id;
        this.manufacturer = device.manufacturer;
        this.product = device.product;
        this.serial_number = device.serial_number;
        this.handle = device.handle;

        this.on('error', () => {});
    }

    isOpened() {
        return addon.isOpened(this);
    }

    open(mode) {
        addon.openDevice(this);
        addon.resetChannel(this, 0);

        if ('termination' in mode)
            addon.setTermination(this, 0, mode['termination']);

        if ('bit_timing' in mode)
            addon.setBitTiming(this, 0, mode['bit_timing']);

        if ('data_bit_timing' in mode)
            addon.setDataBitTiming(this, 0, mode['data_bit_timing']);

        if ('fd' in mode && mode['fd'])
            addon.startChannel(this, 0, 1 << 8);
        else
            addon.startChannel(this, 0, 0);

        (async () => {
            while (this.isOpened()) {
                try {
                    const frame = await addon.receive(this, 0, 1000);
                    this.emit('frame', frame);
                } catch (error) {
                    this.emit('error', error);
                }
            }
        })();
    }

    close() {
        addon.closeDevice(this);
    }

    send(frame) {
        return addon.send({'handle': this.handle}, 0, 1000, frame);
    }

    receive() {
        return once(this, 'frame').then((e) => e[0]);
    }
}

const wrapper = {
    FRAME_TYPE_RX: addon.FRAME_TYPE_RX,
    FRAME_TYPE_EFF: addon.FRAME_TYPE_EFF,
    FRAME_TYPE_RTR: addon.FRAME_TYPE_RTR,
    FRAME_TYPE_ERR: addon.FRAME_TYPE_ERR,
    FRAME_TYPE_FD: addon.FRAME_TYPE_FD,
    FRAME_TYPE_BRS: addon.FRAME_TYPE_BRS,
    FRAME_TYPE_ESI: addon.FRAME_TYPE_ESI,

    listDevice: function listDevice() {
        list = [];
        for (const device of addon.listDevice()) {
            list.push(new CandleDevice(device));
        }
        return list;
    }
}

module.exports = wrapper;