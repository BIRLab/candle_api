const addon = require('bindings')('candle-js');
const { once, EventEmitter } = require('events');

function parseErrorFrame(frame) {
    const errorFlags = [];
    if (frame.can_id & (1 << 0))
        errorFlags.push('TX timeout');
    if (frame.can_id & (1 << 1))
        errorFlags.push('arbitration lost in bit ' + frame.data[0]);
    if (frame.can_id & (1 << 2))
        errorFlags.push('controller problems');
    if (frame.data[1] & (1 << 0))
        errorFlags.push('RX buffer overflow');
    if (frame.data[1] & (1 << 1))
        errorFlags.push('TX buffer overflow');
    if (frame.data[1] & (1 << 2))
        errorFlags.push('reached warning level for RX errors');
    if (frame.data[1] & (1 << 3))
        errorFlags.push('reached warning level for TX errors');
    if (frame.data[1] & (1 << 4))
        errorFlags.push('reached error passive status RX');
    if (frame.data[1] & (1 << 5))
        errorFlags.push('reached error passive status TX');
    if (frame.data[1] & (1 << 6))
        errorFlags.push('recovered to error active state');
    if (frame.can_id & (1 << 3))
        errorFlags.push('protocol violations');
    if (frame.data[2] & (1 << 0))
        errorFlags.push('single bit error');
    if (frame.data[2] & (1 << 1))
        errorFlags.push('frame format error');
    if (frame.data[2] & (1 << 2))
        errorFlags.push('bit stuffing error');
    if (frame.data[2] & (1 << 3))
        errorFlags.push('unable to send dominant bit');
    if (frame.data[2] & (1 << 4))
        errorFlags.push('unable to send recessive bit');
    if (frame.data[2] & (1 << 5))
        errorFlags.push('bus overload');
    if (frame.data[2] & (1 << 6))
        errorFlags.push('active error announcement');
    if (frame.data[2] & (1 << 7))
        errorFlags.push('error occurred on transmission');
    if (frame.data[3] === 0x03)
        errorFlags.push('start of frame');
    if (frame.data[3] === 0x02)
        errorFlags.push('ID bits 28 - 21 (SFF: 10 - 3)');
    if (frame.data[3] === 0x06)
        errorFlags.push('ID bits 20 - 18 (SFF: 2 - 0 )');
    if (frame.data[3] === 0x04)
        errorFlags.push('substitute RTR (SFF: RTR)');
    if (frame.data[3] === 0x05)
        errorFlags.push('identifier extension');
    if (frame.data[3] === 0x07)
        errorFlags.push('ID bits 17-13');
    if (frame.data[3] === 0x0F)
        errorFlags.push('ID bits 12-5');
    if (frame.data[3] === 0x0E)
        errorFlags.push('ID bits 4-0');
    if (frame.data[3] === 0x0C)
        errorFlags.push('RTR');
    if (frame.data[3] === 0x0D)
        errorFlags.push('reserved bit 1');
    if (frame.data[3] === 0x09)
        errorFlags.push('reserved bit 0');
    if (frame.data[3] === 0x0B)
        errorFlags.push('data length code');
    if (frame.data[3] === 0x0A)
        errorFlags.push('data section');
    if (frame.data[3] === 0x08)
        errorFlags.push('CRC sequence');
    if (frame.data[3] === 0x18)
        errorFlags.push('CRC delimiter');
    if (frame.data[3] === 0x19)
        errorFlags.push('ACK slot');
    if (frame.data[3] === 0x1B)
        errorFlags.push('ACK delimiter');
    if (frame.data[3] === 0x1A)
        errorFlags.push('end of frame');
    if (frame.data[3] === 0x12)
        errorFlags.push('intermission');
    if (frame.can_id & (1 << 4))
        errorFlags.push('transceiver status');
    if (frame.data[4] === 0x04)
        errorFlags.push('CANH no wire');
    if (frame.data[4] === 0x05)
        errorFlags.push('CANH short to BAT');
    if (frame.data[4] === 0x06)
        errorFlags.push('CANH short to VCC');
    if (frame.data[4] === 0x07)
        errorFlags.push('CANH short to GND');
    if (frame.data[4] === 0x40)
        errorFlags.push('CANL no wire');
    if (frame.data[4] === 0x50)
        errorFlags.push('CANL short to BAT');
    if (frame.data[4] === 0x60)
        errorFlags.push('CANL short to VCC');
    if (frame.data[4] === 0x70)
        errorFlags.push('CANL short to GND');
    if (frame.data[4] === 0x80)
        errorFlags.push('CANL short to CANH');
    if (frame.can_id & (1 << 5))
        errorFlags.push('received no ACK on transmission');
    if (frame.can_id & (1 << 6))
        errorFlags.push('bus off');
    if (frame.can_id & (1 << 7))
        errorFlags.push('bus error');
    if (frame.can_id & (1 << 8))
        errorFlags.push('controller restarted');
    errorFlags.push('TX error count: ' + frame.data[6]);
    errorFlags.push('RX error count: ' + frame.data[7]);
    return errorFlags.join(', ');
}

class CandleDevice extends EventEmitter {
    constructor(device) {
        super();
        this.vendor_id = device.vendor_id;
        this.product_id = device.product_id;
        this.manufacturer = device.manufacturer;
        this.product = device.product;
        this.serial_number = device.serial_number;
        this.handle = device.handle;

        this.rxErrorCount = 0;
        this.on('error', () => {
            this.rxErrorCount++;
            if (this.rxErrorCount > 99) {
                this.close();
                console.error(`The device is shut down due to too many errors (error count: ${this.rxErrorCount})`);
            }
        });
        this.on('frame', () => {
            this.rxErrorCount = 0;
        })
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
                    if (frame.type & addon.FRAME_TYPE_ERR)
                        this.emit('error', parseErrorFrame(frame));
                    else
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
        const list = [];
        for (const device of addon.listDevice()) {
            list.push(new CandleDevice(device));
        }
        return list;
    }
}

module.exports = wrapper;