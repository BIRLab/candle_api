import candle from './binding.mjs';

// scan device
const dev = candle.listDevice()[0];
console.log(dev);

// open device
dev.open({
    fd: true,
    termination: true,
    bit_timing: {
        "prop_seg": 1,
        "phase_seg1": 43,
        "phase_seg2": 15,
        "sjw": 15,
        "brp": 2
    },
    data_bit_timing: {
        "prop_seg": 1,
        "phase_seg1": 7,
        "phase_seg2": 3,
        "sjw": 3,
        "brp": 2
    }
});

// send frame
dev.send({
    type: candle.FRAME_TYPE_FD | candle.FRAME_TYPE_EFF,
    can_id: 123456,
    can_dlc: 8,
    data: [0, 1, 2, 3, 4, 5, 6, 7]
}).then((frame) => {
    console.log(`Send: ${JSON.stringify(frame)}`);
}).catch((error) => {
    console.error("Error sending frame:", error);
});

// subscribe frame
dev.on('frame', (frame) => {
    console.log(`On frame: ${JSON.stringify(frame)}`);
});

dev.on('error', (error) => {
    console.log(`On error: ${error}`);
});

// polling frame
(async function() {
    while (dev.isOpened()) {
        try {
            const frame = await dev.receive();
            console.log(`Receive: ${JSON.stringify(frame)}`);
        } catch (error) {
            console.error("Error receiving frame:", error);
        }
    }
})();

setTimeout(() => {dev.close()}, 5000);
