from typing import Sequence


class CandleMode:
    def __init__(self, listen_only: bool = False, loop_back: bool = False, triple_sample: bool = False, one_shot: bool = False, hardware_timestamp: bool = False, pad_package: bool = False, fd: bool = False, bit_error_reporting: bool = False) -> None:
        ...


class CandleFrameType:
    def __init__(self, rx: bool = False, extended_id: bool = False, remote_frame: bool = False, error_frame: bool = False, fd: bool = False, bitrate_switch: bool = False, error_state_indicator: bool = False) -> None:
        self.rx = rx
        self.extended_id = extended_id
        self.remote_frame = remote_frame
        self.error_frame = error_frame
        self.fd = fd
        self.bitrate_switch = bitrate_switch
        self.error_state_indicator = error_state_indicator


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
    def nominal_bit_timing(self) -> CandleBitTimingConst:
        ...

    @property
    def data_bit_timing(self) -> CandleBitTimingConst:
        ...


class CandleDevice(Sequence[CandleChannel]):

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
    def channel_count(self) -> int:
        ...

    @property
    def software_version(self) -> int:
        ...

    @property
    def hardware_version(self) -> int:
        ...

    def open(self) -> bool:
        ...

    def close(self) -> None:
        ...
