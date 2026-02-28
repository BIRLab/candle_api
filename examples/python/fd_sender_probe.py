#!/usr/bin/env python3
"""Send CAN-FD frames with candle_api at 500K/2M and report sender status."""

from __future__ import annotations

import argparse
import time
from dataclasses import dataclass

try:
    import candle_api
except ModuleNotFoundError as exc:
    raise SystemExit(
        "candle_api Python module not found. Use the project venv, e.g. "
        "'.venv-macos-test2/bin/python examples/python/fd_sender_probe.py ...'"
    ) from exc

if not hasattr(candle_api, "list_device"):
    raise SystemExit(
        "Imported a shadowed 'candle_api' module without bindings. "
        "Do not set PYTHONPATH='.' here; run with the project venv Python instead."
    )


@dataclass
class Timing:
    prop_seg: int
    phase_seg1: int
    phase_seg2: int
    sjw: int
    brp: int
    bitrate: int
    sample_point_permille: int
    tq_ns: int


def _linux_default_sample_point(bitrate: int) -> int:
    # Linux can_calc_bittiming() CiA defaults.
    if bitrate > 800_000:
        return 750
    if bitrate > 500_000:
        return 800
    return 875


def _update_sample_point(
    bt_const: candle_api.CandleBitTimingConst,
    sample_point_nominal: int,
    tseg: int,
) -> tuple[int, int, int, int]:
    # Ported from Linux can_update_sample_point().
    best_sample_point = 0
    best_sample_point_error = 0x7FFFFFFF
    best_tseg1 = 0
    best_tseg2 = 0

    for i in (0, 1):
        tseg2 = tseg + 1 - (sample_point_nominal * (tseg + 1)) // 1000 - i
        tseg2 = max(bt_const.tseg2_min, min(bt_const.tseg2_max, tseg2))
        tseg1 = tseg - tseg2
        if tseg1 > bt_const.tseg1_max:
            tseg1 = bt_const.tseg1_max
            tseg2 = tseg - tseg1

        sample_point = (1000 * (tseg + 1 - tseg2)) // (tseg + 1)
        sample_point_error = abs(sample_point_nominal - sample_point)
        if sample_point <= sample_point_nominal and sample_point_error < best_sample_point_error:
            best_sample_point = sample_point
            best_sample_point_error = sample_point_error
            best_tseg1 = tseg1
            best_tseg2 = tseg2

    return best_sample_point, best_tseg1, best_tseg2, best_sample_point_error


def calc_bittiming(
    bt_const: candle_api.CandleBitTimingConst,
    bitrate: int,
    fclk: int,
    sample_point_permille: int | None = None,
) -> Timing:
    # Ported from Linux can_calc_bittiming(), including tseg odd/even rounding.
    sample_point_nominal = sample_point_permille or _linux_default_sample_point(bitrate)
    brp_step = max(bt_const.brp_inc, 1)

    best_bitrate_error = 0x7FFFFFFF
    best_sample_point_error = 0x7FFFFFFF
    best_tseg = 0
    best_brp = 0

    tseg_start = (bt_const.tseg1_max + bt_const.tseg2_max) * 2 + 1
    tseg_stop = (bt_const.tseg1_min + bt_const.tseg2_min) * 2

    for tseg in range(tseg_start, tseg_stop - 1, -1):
        tseg_all = 1 + tseg // 2
        brp = fclk // (tseg_all * bitrate) + (tseg % 2)
        brp = (brp // brp_step) * brp_step
        if brp < bt_const.brp_min or brp > bt_const.brp_max:
            continue

        actual_bitrate = fclk // (brp * tseg_all)
        bitrate_error = abs(bitrate - actual_bitrate)
        if bitrate_error > best_bitrate_error:
            continue
        if bitrate_error < best_bitrate_error:
            best_sample_point_error = 0x7FFFFFFF

        _, _, _, sample_point_error = _update_sample_point(bt_const, sample_point_nominal, tseg // 2)
        if sample_point_error > best_sample_point_error:
            continue

        best_bitrate_error = bitrate_error
        best_sample_point_error = sample_point_error
        best_tseg = tseg // 2
        best_brp = brp
        if bitrate_error == 0 and sample_point_error == 0:
            break

    if best_brp == 0:
        raise RuntimeError(f"Cannot calculate timing for bitrate={bitrate}")

    sample_point, tseg1, tseg2, _ = _update_sample_point(bt_const, sample_point_nominal, best_tseg)
    prop_seg = tseg1 // 2
    phase_seg1 = tseg1 - prop_seg
    # Match Linux/iproute2 style defaults: choose ~half of phase_seg2 when unspecified.
    sjw = max(1, min(bt_const.sjw_max if bt_const.sjw_max > 0 else 1, tseg2 // 2))
    actual_bitrate = fclk // (best_brp * (1 + tseg1 + tseg2))
    tq_ns = (best_brp * 1_000_000_000) // fclk

    return Timing(
        prop_seg=prop_seg,
        phase_seg1=phase_seg1,
        phase_seg2=tseg2,
        sjw=sjw,
        brp=best_brp,
        bitrate=actual_bitrate,
        sample_point_permille=sample_point,
        tq_ns=tq_ns,
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--nominal", type=int, default=500_000)
    parser.add_argument("--data", type=int, default=2_000_000)
    parser.add_argument("--id", type=lambda s: int(s, 0), default=0x123)
    parser.add_argument("--count", type=int, default=20)
    parser.add_argument("--interval-ms", type=int, default=100)
    parser.add_argument("--loopback", action="store_true")
    parser.add_argument("--debug-state", action="store_true", help="Print CAN controller state after each timeout.")
    parser.add_argument("--no-brs", action="store_true", help="Disable bit rate switching (use nominal rate for whole FD frame).")
    args = parser.parse_args()

    devices = candle_api.list_device()
    if not devices:
        raise SystemExit("No candle_api devices found")
    dev = devices[0]
    print(
        f"Using device vid:pid={dev.vendor_id:04x}:{dev.product_id:04x} "
        f"product={dev.product!r} manufacturer={dev.manufacturer!r}"
    )
    dev.open()
    try:
        ch = dev[0]
        feat = ch.feature
        if not feat.fd:
            raise SystemExit("Selected channel does not advertise CAN-FD support")

        fclk = ch.clock_frequency
        nominal = calc_bittiming(ch.nominal_bit_timing_const, args.nominal, fclk)
        data = calc_bittiming(ch.data_bit_timing_const, args.data, fclk)
        print(f"Nominal timing: {nominal}")
        print(f"Data timing:    {data}")

        ch.set_bit_timing(nominal.prop_seg, nominal.phase_seg1, nominal.phase_seg2, nominal.sjw, nominal.brp)
        ch.set_data_bit_timing(data.prop_seg, data.phase_seg1, data.phase_seg2, data.sjw, data.brp)
        ch.start(
            listen_only=False,
            loop_back=args.loopback and feat.loop_back,
            triple_sample=False,
            one_shot=False,
            hardware_timestamp=feat.hardware_timestamp,
            pad_package=False,
            fd=True,
            bit_error_reporting=False,
        )
        print(f"Channel started: fd=True loopback={args.loopback and feat.loop_back}")

        payload = bytes(range(12))
        frame = candle_api.CandleCanFrame(
            candle_api.CandleFrameType(rx=False, fd=True, bitrate_switch=not args.no_brs),
            args.id,
            9,  # 12-byte CAN-FD payload (cannot be classical CAN)
            payload,
        )

        tx_ok = 0
        tx_fail = 0
        rx_seen = 0
        for i in range(args.count):
            try:
                ch.send(frame, 0.25)
                tx_ok += 1
                brs = 0 if args.no_brs else 1
                print(f"TX[{i+1}/{args.count}] ok id=0x{args.id:X} dlc=9 fd=1 brs={brs}")
            except TimeoutError:
                tx_fail += 1
                print(f"TX[{i+1}/{args.count}] timeout (likely no ACK on bus)")
                if args.debug_state:
                    try:
                        st = ch.state
                        print(
                            "  state: "
                            f"txerr={st.tx_error_count} rxerr={st.rx_error_count} "
                            f"active={st.state.error_active} warning={st.state.error_warning} "
                            f"passive={st.state.error_passive} bus_off={st.state.bus_off}"
                        )
                    except RuntimeError as exc:
                        print(f"  state: unavailable ({exc})")
            if args.loopback:
                try:
                    rx = ch.receive(0.05)
                    print(
                        f"RX loopback id=0x{rx.can_id:X} fd={rx.frame_type.fd} "
                        f"brs={rx.frame_type.bitrate_switch} len={len(bytes(rx.data))}"
                    )
                    rx_seen += 1
                except TimeoutError:
                    pass
            time.sleep(max(args.interval_ms, 0) / 1000.0)

        print(f"Summary: tx_ok={tx_ok} tx_fail={tx_fail} rx_loopback={rx_seen}")
        if tx_ok == 0:
            return 2
        return 0
    finally:
        dev.close()


if __name__ == "__main__":
    raise SystemExit(main())
