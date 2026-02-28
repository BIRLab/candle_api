#!/usr/bin/env python3
"""Probe gs_usb CAN FD configuration at 500K/2M using candle_api."""

from __future__ import annotations

import sys
import time
from dataclasses import dataclass

import candle_api


GS_USB_IDS = {
    (0x1D50, 0x606F),  # CANable/candleLight
    (0x1D50, 0x606D),  # CANtact
    (0x1D50, 0x6070),  # CANable 2.0
    (0x1209, 0x2323),  # candleLight
    (0x1209, 0x0001),  # Geschwister Schneider
    (0x16D0, 0x0F30),  # Innomaker USB2CAN
}


@dataclass
class Timing:
    prop_seg: int
    phase_seg1: int
    phase_seg2: int
    sjw: int
    brp: int
    bitrate: int


def calc_bittiming(bt_const: candle_api.CandleBitTimingConst, bitrate: int, fclk: int) -> Timing:
    for brp in range(bt_const.brp_min, bt_const.brp_max + 1):
        if bt_const.brp_inc > 1 and (brp - bt_const.brp_min) % bt_const.brp_inc != 0:
            continue

        tq = fclk // brp
        if tq == 0 or (tq % bitrate) != 0:
            continue

        btq = tq // bitrate
        if btq < 8 or btq > 25:
            continue

        prop_seg = 1
        remaining = btq - 1 - prop_seg

        for tseg1 in range(bt_const.tseg1_min, bt_const.tseg1_max + 1):
            if tseg1 > remaining:
                break
            tseg2 = remaining - tseg1
            if tseg2 < bt_const.tseg2_min or tseg2 > bt_const.tseg2_max:
                continue

            sjw = max(1, min(tseg2, bt_const.sjw_max))
            actual = fclk // (brp * (1 + prop_seg + tseg1 + tseg2))
            if actual == bitrate:
                return Timing(prop_seg, tseg1, tseg2, sjw, brp, actual)

    raise RuntimeError(f"Cannot calculate bit timing for bitrate={bitrate} with fclk={fclk}")


def find_gs_usb_device():
    devices = candle_api.list_device()
    if not devices:
        raise RuntimeError("No CAN devices found via candle_api.list_device()")

    for idx, dev in enumerate(devices):
        vid_pid = (dev.vendor_id, dev.product_id)
        print(
            f"[device {idx}] vid:pid={dev.vendor_id:04x}:{dev.product_id:04x} "
            f"product={dev.product!r} manufacturer={dev.manufacturer!r}"
        )
        if vid_pid in GS_USB_IDS:
            return dev

    raise RuntimeError("No known gs_usb-compatible adapter found")


def main() -> int:
    dev = find_gs_usb_device()
    dev.open()

    try:
        ch = dev[0]
        feature = ch.feature
        print(
            f"[channel 0] feature: fd={feature.fd} loop_back={feature.loop_back} "
            f"hw_timestamp={feature.hardware_timestamp} termination={feature.termination}"
        )
        if not feature.fd:
            raise RuntimeError("Adapter/channel does not advertise CAN FD support")

        fclk = ch.clock_frequency
        nominal = calc_bittiming(ch.nominal_bit_timing_const, 500_000, fclk)
        data = calc_bittiming(ch.data_bit_timing_const, 2_000_000, fclk)
        print(f"[timing nominal] {nominal}")
        print(f"[timing data]    {data}")

        ch.set_bit_timing(
            nominal.prop_seg,
            nominal.phase_seg1,
            nominal.phase_seg2,
            nominal.sjw,
            nominal.brp,
        )
        ch.set_data_bit_timing(
            data.prop_seg,
            data.phase_seg1,
            data.phase_seg2,
            data.sjw,
            data.brp,
        )

        ch.start(
            listen_only=False,
            loop_back=feature.loop_back,
            triple_sample=False,
            one_shot=False,
            hardware_timestamp=feature.hardware_timestamp,
            pad_package=False,
            fd=True,
            bit_error_reporting=False,
        )
        print("[channel 0] started in CAN FD mode")

        payload = bytes(range(12))
        tx = candle_api.CandleCanFrame(
            candle_api.CandleFrameType(rx=False, fd=True, bitrate_switch=True),
            0x123,
            9,  # DLC 9 -> 12 bytes, requires CAN FD
            payload,
        )

        ch.send(tx, 0.5)
        print("[tx] sent CAN FD+BRS loopback frame")

        deadline = time.time() + 2.0
        while time.time() < deadline:
            try:
                rx = ch.receive(0.25)
            except TimeoutError:
                continue

            rx_data = bytes(rx.data)
            print(
                f"[rx] id=0x{rx.can_id:X} dlc={rx.can_dlc} "
                f"fd={rx.frame_type.fd} brs={rx.frame_type.bitrate_switch} data={rx_data.hex()}"
            )
            if rx.frame_type.rx and rx.frame_type.fd and rx.frame_type.bitrate_switch and rx_data == payload:
                print("PASS: CAN FD 500K/2M configuration and loopback verified")
                return 0

        raise RuntimeError("No matching CAN FD loopback frame received")
    finally:
        dev.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # noqa: BLE001
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
