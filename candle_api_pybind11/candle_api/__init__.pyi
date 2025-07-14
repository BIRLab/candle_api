from typing import Optional
from collections.abc import Buffer


class CandleFrameType:
    def __init__(self, rx: bool = False, extended_id: bool = False, remote_frame: bool = False, error_frame: bool = False, fd: bool = False, bitrate_switch: bool = False, error_state_indicator: bool = False) -> None:
        ...

    @property
    def rx(self) -> bool:
        ...

    @property
    def extended_id(self) -> bool:
        ...

    @property
    def remote_frame(self) -> bool:
        ...

    @property
    def error_frame(self) -> bool:
        ...

    @property
    def fd(self) -> bool:
        ...

    @property
    def bitrate_switch(self) -> bool:
        ...

    @property
    def error_state_indicator(self) -> bool:
        ...


class CandleCanFrame:
    def __init__(self, frame_type: CandleFrameType, can_id: int, can_dlc: int, data: Buffer) -> None:
        ...

    def __buffer__(self, flags: int) -> memoryview:
        ...

    def __release_buffer__(self, view: memoryview) -> None:
        ...

    @property
    def frame_type(self) -> CandleFrameType:
        ...

    @property
    def can_id(self) -> int:
        ...

    @property
    def can_dlc(self) -> int:
        ...

    @property
    def size(self) -> int:
        ...

    @property
    def data(self) -> bytes:
        ...

    @property
    def timestamp_us(self) -> int:
        ...

    @property
    def timestamp(self) -> float:
        ...


class CandleCanState:
    @property
    def error_active(self) -> bool:
        ...

    @property
    def error_warning(self) -> bool:
        ...

    @property
    def error_passive(self) -> bool:
        ...

    @property
    def bus_off(self) -> bool:
        ...

    @property
    def stopped(self) -> bool:
        ...

    @property
    def sleeping(self) -> bool:
        ...


class CandleState:
    @property
    def state(self) -> CandleCanState:
        ...

    @property
    def rx_error_count(self) -> int:
        ...

    @property
    def tx_error_count(self) -> int:
        ...


class CandleFeature:
    @property
    def listen_only(self) -> bool:
        ...

    @property
    def loop_back(self) -> bool:
        ...

    @property
    def triple_sample(self) -> bool:
        ...

    @property
    def one_shot(self) -> bool:
        ...

    @property
    def hardware_timestamp(self) -> bool:
        ...

    @property
    def pad_package(self) -> bool:
        ...

    @property
    def fd(self) -> bool:
        ...

    @property
    def bit_error_reporting(self) -> bool:
        ...

    @property
    def termination(self) -> bool:
        ...

    @property
    def get_state(self) -> bool:
        ...


class CandleBitTimingConst:
    @property
    def tseg1_min(self) -> int:
        ...

    @property
    def tseg1_max(self) -> int:
        ...

    @property
    def tseg2_min(self) -> int:
        ...

    @property
    def tseg2_max(self) -> int:
        ...

    @property
    def sjw_max(self) -> int:
        ...

    @property
    def brp_min(self) -> int:
        ...

    @property
    def brp_max(self) -> int:
        ...

    @property
    def brp_inc(self) -> int:
        ...


class CandleChannel:
    @property
    def feature(self) -> CandleFeature:
        ...

    @property
    def clock_frequency(self) -> int:
        ...

    @property
    def nominal_bit_timing_const(self) -> CandleBitTimingConst:
        ...

    @property
    def data_bit_timing_const(self) -> CandleBitTimingConst:
        ...

    @property
    def state(self) -> CandleState:
        ...

    @property
    def termination(self) -> bool:
        ...

    def reset(self) -> None:
        ...

    def start(self, listen_only: bool = False, loop_back: bool = False, triple_sample: bool = False, one_shot: bool = False, hardware_timestamp: bool = False, pad_package: bool = False, fd: bool = False, bit_error_reporting: bool = False) -> None:
        ...

    def set_bit_timing(self, prop_seg: int, phase_seg1: int, phase_seg2: int, sjw: int, brp: int) -> None:
        ...

    def set_data_bit_timing(self, prop_seg: int, phase_seg1: int, phase_seg2: int, sjw: int, brp: int) -> None:
        ...

    def set_termination(self, enable: bool) -> None:
        ...

    def send_nowait(self, frame: CandleCanFrame) -> None:
        ...

    def receive_nowait(self) -> Optional[CandleCanFrame]:
        ...

    def send(self, frame: CandleCanFrame, timeout: float) -> None:
        ...

    def receive(self, timeout: float) -> CandleCanFrame:
        ...


class CandleDevice:

    def __len__(self) -> int:
        ...

    def __getitem__(self, index: int) -> CandleChannel:
        ...

    @property
    def is_connected(self) -> bool:
        ...

    @property
    def is_open(self) -> bool:
        ...

    @property
    def vendor_id(self) -> int:
        ...

    @property
    def product_id(self) -> int:
        ...

    @property
    def manufacturer(self) -> str:
        ...

    @property
    def product(self) -> str:
        ...

    @property
    def serial_number(self) -> str:
        ...

    @property
    def channel_count(self) -> int:
        ...

    @property
    def software_version(self) -> int:
        ...

    @property
    def hardware_version(self) -> int:
        ...

    def open(self) -> None:
        ...

    def close(self) -> None:
        ...


def list_device() -> list[CandleDevice]:
    ...
