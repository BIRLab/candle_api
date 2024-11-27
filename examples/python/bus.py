import can
from candle_api.candle_bus import CandleBus

bus: CandleBus  # This line is added to provide type hints.

# Create a CandleBus instance in the python-can API.
with can.Bus(interface='candle_bus', channel=0, fd=True, bitrate=1000000, data_bitrate=5000000, ignore_config=True) as bus:
    # Note that bus is an instance of CandleBus.
    assert isinstance(bus, CandleBus)

    # Send normal can message without data.
    bus.send(can.Message(arbitration_id=1, is_extended_id=False))

    # Send normal can message with extended id
    bus.send(can.Message(arbitration_id=2, is_extended_id=True))

    # Send normal can message with data.
    bus.send(can.Message(arbitration_id=3, is_extended_id=False, data=[i for i in range(8)]))

    # Send can fd message.
    if bus._channel.feature.fd:
        bus.send(can.Message(arbitration_id=4, is_extended_id=False, is_fd=True, bitrate_switch=True,
                             error_state_indicator=True, data=[i for i in range(64)]))

    # Read messages from bus.
    for message in bus:
        print(message)
