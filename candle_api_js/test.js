var addon = require('bindings')('candle-js');

for (d of addon.listDevice()) {
    console.log(`idVender: ${d.idVender}`);
    console.log(`idProduct: ${d.idProduct}`);
    console.log(`manufacture: ${d.manufacture}`);
    console.log(`product: ${d.product}`);
    console.log(`serialNumber: ${d.serialNumber}`);

    d.open({
        "bit_timing": {
            "prop_seg": 1,
            "phase_seg1": 43,
            "phase_seg2": 15,
            "sjw": 15,
            "brp": 2
        },
        "data_bit_timing": {
            "prop_seg": 1,
            "phase_seg1": 7,
            "phase_seg2": 3,
            "sjw": 3,
            "brp": 2
        }
    });

    d.send({
        "id": 0,
        "dlc": 8,
        "data": [0, 1, 2, 3, 4, 5, 6, 7]
    }).then((result) => {
        console.log(result);
    }).catch((e) => {
        console.log(e);
    });

    d.receive()
        .then((frame) => {
            console.log(frame);
        })
        .catch((e) => {
            console.log(e);
        });
}
