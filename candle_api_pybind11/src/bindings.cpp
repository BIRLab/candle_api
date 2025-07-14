#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <candle_api.h>

namespace py = pybind11;

static const uint8_t dlc2len[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};

struct CandleContext {
    CandleContext() {
        candle_initialize();
    }

    ~CandleContext() {
        candle_finalize();
    }
};

class CandleDeviceReference {
public:
    explicit CandleDeviceReference(candle_device* device): device_(device) {
        candle_ref_device(device_);
    }

    CandleDeviceReference(const CandleDeviceReference& other): device_(other.device_) {
        candle_ref_device(device_);
    }

    CandleDeviceReference(CandleDeviceReference&& other) noexcept : device_(other.device_) {
        other.device_ = nullptr;
    }

    ~CandleDeviceReference() {
        if (device_ != nullptr)
            candle_unref_device(device_);
    }

protected:
    candle_device* device_;
};

class CandleFrameType {
public:
    explicit CandleFrameType(const candle_frame_type& ft): ft_(ft) { }

    CandleFrameType(bool rx, bool extended_id, bool remote_frame, bool error_frame, bool fd, bool bitrate_switch, bool error_state_indicator) {
        int ft = 0;
        if (rx)
            ft |= CANDLE_FRAME_TYPE_RX;
        if (extended_id)
            ft |= CANDLE_FRAME_TYPE_EFF;
        if (remote_frame)
            ft |= CANDLE_FRAME_TYPE_RTR;
        if (error_frame)
            ft |= CANDLE_FRAME_TYPE_ERR;
        if (fd)
            ft |= CANDLE_FRAME_TYPE_FD;
        if (bitrate_switch)
            ft |= CANDLE_FRAME_TYPE_BRS;
        if (error_state_indicator)
            ft |= CANDLE_FRAME_TYPE_ESI;

        ft_ = static_cast<candle_frame_type>(ft);
    }

    bool getRx() {
        return ft_ & CANDLE_FRAME_TYPE_RX;
    }

    bool getExtendedId() {
        return ft_ & CANDLE_FRAME_TYPE_EFF;
    }

    bool getRemoteFrame() {
        return ft_ & CANDLE_FRAME_TYPE_RTR;
    }

    bool getErrorFrame() {
        return ft_ & CANDLE_FRAME_TYPE_ERR;
    }

    bool getFD() {
        return ft_ & CANDLE_FRAME_TYPE_FD;
    }

    bool getBitrateSwitch() {
        return ft_ & CANDLE_FRAME_TYPE_BRS;
    }

    bool getErrorStateIndicator() {
        return ft_ & CANDLE_FRAME_TYPE_ESI;
    }

private:
    candle_frame_type ft_;

    friend class CandleCanFrame;
};

class CandleCanFrame {
public:
    explicit CandleCanFrame(const candle_can_frame& frame): frame_(frame) { }

    CandleCanFrame(const CandleFrameType& frame_type, uint32_t can_id, uint8_t can_dlc, const py::buffer& data): frame_() {
        if (can_dlc > 15)
            throw py::value_error("DLC can only be between 0 and 15");
        size_t required_data_len = dlc2len[can_dlc];

        py::buffer_info info = data.request();
        size_t total_data_len = info.itemsize * info.size;

        if (total_data_len < required_data_len)
            throw py::value_error("Data length is smaller than can dlc");

        frame_.type = frame_type.ft_;
        frame_.can_id = can_id;
        frame_.can_dlc = can_dlc;

        std::memcpy(frame_.data, info.ptr, required_data_len);
    }

    CandleFrameType getFrameType() {
        return CandleFrameType(frame_.type);
    }

    uint32_t getCanId() {
        return frame_.can_id;
    }

    uint8_t getCanDLC() {
        return frame_.can_dlc;
    }

    py::bytes getData() {
        return { reinterpret_cast<char*>(frame_.data), dlc2len[frame_.can_dlc] };
    }

    size_t getSize() {
        return dlc2len[frame_.can_dlc];
    }

    uint32_t getTimestampUs() {
        return frame_.timestamp_us;
    }

    double getTimestamp() {
        return frame_.timestamp_us / 1e6;
    }

    py::buffer_info getBuffer() {
        return py::buffer_info(
            frame_.data,
            1,
            py::format_descriptor<uint8_t>::format(),
            1,
            { dlc2len[frame_.can_dlc] },
            { 1 }
        );
    }

private:
    candle_can_frame frame_;

    friend class CandleChannel;
};

class CandleFeature {
public:
    explicit CandleFeature(const candle_feature& feature): feature_(feature) { }

    bool getListenOnly() {
        return feature_ & CANDLE_FEATURE_LISTEN_ONLY;
    }

    bool getLoopBack() {
        return feature_ & CANDLE_FEATURE_LOOP_BACK;
    }

    bool getTripleSample() {
        return feature_ & CANDLE_FEATURE_TRIPLE_SAMPLE;
    }

    bool getOneShot() {
        return feature_ & CANDLE_FEATURE_ONE_SHOT;
    }

    bool getHardwareTimestamp() {
        return feature_ & CANDLE_FEATURE_HW_TIMESTAMP;
    }

    bool getPadPackage() {
        return feature_ & CANDLE_FEATURE_PAD_PKTS_TO_MAX_PKT_SIZE;
    }

    bool getFD() {
        return feature_ & CANDLE_FEATURE_FD;
    }

    bool getBitErrorReporting() {
        return feature_ & CANDLE_FEATURE_BERR_REPORTING;
    }

    bool getTermination() {
        return feature_ & CANDLE_FEATURE_TERMINATION;
    }

    bool getGetState() {
        return feature_ & CANDLE_FEATURE_GET_STATE;
    }

private:
    candle_feature feature_;
};

class CandleBitTimingConst {
public:
    explicit CandleBitTimingConst(const candle_bit_timing_const& bt): bt_(bt) { }

    uint32_t getTseg1Min() {
        return bt_.tseg1_min;
    }

    uint32_t getTseg1Max() {
        return bt_.tseg1_max;
    }

    uint32_t getTseg2Min() {
        return bt_.tseg2_min;
    }

    uint32_t getTseg2Max() {
        return bt_.tseg2_max;
    }

    uint32_t getSjwMax() {
        return bt_.sjw_max;
    }

    uint32_t getBrpMin() {
        return bt_.brp_min;
    }

    uint32_t getBrpMax() {
        return bt_.brp_max;
    }

    uint32_t getBrpInc() {
        return bt_.brp_inc;
    }

private:
    candle_bit_timing_const bt_;
};

class CandleCanState {
public:
    explicit CandleCanState(const candle_can_state& cst): cst_(cst) { }

    bool getErrorActivate() {
        return cst_ & CANDLE_CAN_STATE_ERROR_ACTIVE;
    }

    bool getErrorWarning() {
        return cst_ & CANDLE_CAN_STATE_ERROR_WARNING;
    }

    bool getErrorPassive() {
        return cst_ & CANDLE_CAN_STATE_ERROR_PASSIVE;
    }

    bool getBusOff() {
        return cst_ & CANDLE_CAN_STATE_BUS_OFF;
    }

    bool getStopped() {
        return cst_ & CANDLE_CAN_STATE_STOPPED;
    }

    bool getSleeping() {
        return cst_ & CANDLE_CAN_STATE_SLEEPING;
    }

private:
    candle_can_state cst_;
};

class CandleState {
public:
    explicit CandleState(const candle_state& st): st_(st) { }

    CandleCanState getState() {
        return CandleCanState(st_.state);
    }

    uint32_t getRxErrorCount() {
        return st_.rxerr;
    }

    uint32_t getTxErrorCount() {
        return st_.txerr;
    }

private:
    candle_state st_;
};

class CandleChannel: public CandleDeviceReference {
public:
    explicit CandleChannel(candle_device* device, uint8_t index): CandleDeviceReference(device), index_(index) { }

    CandleFeature getFeature() {
        return CandleFeature(device_->channels[index_].feature);
    }

    uint32_t getClockFrequency() {
        return device_->channels[index_].clock_frequency;
    }

    CandleBitTimingConst getNominalBitTimingConst() {
        return CandleBitTimingConst(device_->channels[index_].bit_timing_const.nominal);
    }

    CandleBitTimingConst getDataBitTimingConst() {
        return CandleBitTimingConst(device_->channels[index_].bit_timing_const.data);
    }

    CandleState getState() {
        candle_state st;
        if (!candle_get_state(device_, index_, &st))
            throw std::runtime_error("Cannot get state");
        return CandleState(st);
    }

    bool getTermination() {
        bool enable;

        if (!candle_get_termination(device_, index_, &enable))
            throw std::runtime_error("Cannot read termination");

        return enable;
    }

    void setTermination(bool enable) {
        if (!candle_set_termination(device_, index_, enable))
            throw std::runtime_error("Cannot set termination");
    }

    void reset() {
        if (!candle_reset_channel(device_, index_))
            throw std::runtime_error("Cannot reset channel");
    }

    void start(bool listen_only, bool loop_back, bool triple_sample, bool one_shot, bool hardware_timestamp, bool pad_package, bool fd, bool bit_error_reporting) {
        int mode = CANDLE_MODE_NORMAL;

        if (listen_only)
            mode |= CANDLE_MODE_LISTEN_ONLY;
        if (loop_back)
            mode |= CANDLE_MODE_LOOP_BACK;
        if (triple_sample)
            mode |= CANDLE_MODE_TRIPLE_SAMPLE;
        if (one_shot)
            mode |= CANDLE_MODE_ONE_SHOT;
        if (hardware_timestamp)
            mode |= CANDLE_MODE_HW_TIMESTAMP;
        if (pad_package)
            mode |= CANDLE_MODE_PAD_PKTS_TO_MAX_PKT_SIZE;
        if (fd)
            mode |= CANDLE_MODE_FD;
        if (bit_error_reporting)
            mode |= CANDLE_MODE_BERR_REPORTING;

        if (!candle_start_channel(device_, index_, static_cast<candle_mode>(mode)))
            throw std::runtime_error("Cannot start channel");
    }

    void setBitTiming(uint32_t prop_seg, uint32_t phase_seg1, uint32_t phase_seg2, uint32_t sjw, uint32_t brp) {
        candle_bit_timing bt = { prop_seg, phase_seg1, phase_seg2, sjw, brp };

        if (!candle_set_bit_timing(device_, index_, &bt))
            throw std::runtime_error("Cannot set bit timing");
    }

    void setDataBitTiming(uint32_t prop_seg, uint32_t phase_seg1, uint32_t phase_seg2, uint32_t sjw, uint32_t brp) {
        candle_bit_timing bt = { prop_seg, phase_seg1, phase_seg2, sjw, brp };

        if (!candle_set_data_bit_timing(device_, index_, &bt))
            throw std::runtime_error("Cannot set data bit timing");
    }

    void sendNowait(CandleCanFrame& frame) {
        if (!candle_send_frame_nowait(device_, index_, &frame.frame_))
            throw std::runtime_error("Cannot send frame");
    }

    std::optional<CandleCanFrame> receiveNowait() {
        candle_can_frame frame;
        if (!candle_receive_frame_nowait(device_, index_, &frame))
            return std::nullopt;
        return CandleCanFrame(frame);
    }

    void send(CandleCanFrame& frame, float timeout) {
        bool ret;

        {
            py::gil_scoped_release release;
            ret = candle_send_frame(device_, index_, &frame.frame_, (uint32_t)(1000 * timeout));
        }

        if (!ret) {
            PyErr_SetString(PyExc_TimeoutError, "Send timeout");
            throw py::error_already_set();
        }
    }

    CandleCanFrame receive(float timeout) {
        candle_can_frame frame;
        bool ret;

        {
            py::gil_scoped_release release;
            ret = candle_receive_frame(device_, index_, &frame, (uint32_t)(1000 * timeout));
        }

        if (!ret) {
            PyErr_SetString(PyExc_TimeoutError, "Receive timeout");
            throw py::error_already_set();
        }

        return CandleCanFrame(frame);
    }

private:
    uint8_t index_;
};

class CandleDevice: public CandleDeviceReference {
public:
    using CandleDeviceReference::CandleDeviceReference;

    bool getIsConnected() {
        return device_->is_connected;
    }

    bool getIsOpen() {
        return device_->is_open;
    }

    uint16_t getVendorId() {
        return device_->vendor_id;
    }

    uint16_t getProductId() {
        return device_->product_id;
    }

    std::string getManufacturer() {
        return device_->manufacturer;
    }

    std::string getProduct() {
        return device_->product;
    }

    std::string getSerialNumber() {
        return device_->serial_number;
    }

    uint8_t getChannelCount() {
        return device_->channel_count;
    }

    uint32_t getSoftwareVersion() {
        return device_->software_version;
    }

    uint32_t getHardwareVersion() {
        return device_->hardware_version;
    }

    void open() {
        if (!candle_open_device(device_))
            throw std::runtime_error("Cannot open device");
        candle_unref_device(device_);
    }

    void close() {
        if (device_->is_open) {
            candle_ref_device(device_);
            candle_close_device(device_);
        }
    }

    CandleChannel getChannel(int index) {
        if (0 <= index && index < device_->channel_count)
            return CandleChannel(device_, index);
        else
            throw py::index_error();
    }
};

std::vector<CandleDevice> list_device() {
    candle_device **device_list;
    size_t device_list_size;
    if (!candle_get_device_list(&device_list, &device_list_size))
        throw std::runtime_error("Cannot get device list");

    std::vector<CandleDevice> list;

    for (int i = 0; i < device_list_size; ++i) {
        list.emplace_back(device_list[i]);
    }

    candle_free_device_list(device_list);

    return list;
}

PYBIND11_MODULE(bindings, m) {
    static CandleContext ctx;

    py::class_<CandleFrameType>(m, "CandleFrameType")
        .def(py::init<bool, bool, bool, bool, bool, bool, bool>(), py::arg("rx") = false, py::arg("extended_id") = false, py::arg("remote_frame") = false, py::arg("error_frame") = false, py::arg("fd") = false, py::arg("bitrate_switch") = false, py::arg("error_state_indicator") = false)
        .def_property_readonly("rx", &CandleFrameType::getRx)
        .def_property_readonly("extended_id", &CandleFrameType::getExtendedId)
        .def_property_readonly("remote_frame", &CandleFrameType::getRemoteFrame)
        .def_property_readonly("error_frame", &CandleFrameType::getErrorFrame)
        .def_property_readonly("fd", &CandleFrameType::getFD)
        .def_property_readonly("bitrate_switch", &CandleFrameType::getBitrateSwitch)
        .def_property_readonly("error_state_indicator", &CandleFrameType::getErrorStateIndicator);

    py::class_<CandleCanFrame>(m, "CandleCanFrame", py::buffer_protocol())
        .def(py::init<const CandleFrameType&, uint32_t, uint8_t, const py::buffer&>(), py::arg("frame_type"), py::arg("can_id"), py::arg("can_dlc"), py::arg("data"))
        .def_property_readonly("frame_type", &CandleCanFrame::getFrameType)
        .def_property_readonly("can_id", &CandleCanFrame::getCanId)
        .def_property_readonly("can_dlc", &CandleCanFrame::getCanDLC)
        .def_property_readonly("size", &CandleCanFrame::getSize)
        .def_property_readonly("data", &CandleCanFrame::getData)
        .def_property_readonly("timestamp_us", &CandleCanFrame::getTimestampUs)
        .def_property_readonly("timestamp", &CandleCanFrame::getTimestamp)
        .def_buffer(&CandleCanFrame::getBuffer);

    py::class_<CandleFeature>(m, "CandleFeature")
        .def_property_readonly("listen_only", &CandleFeature::getListenOnly)
        .def_property_readonly("loop_back", &CandleFeature::getLoopBack)
        .def_property_readonly("triple_sample", &CandleFeature::getTripleSample)
        .def_property_readonly("one_shot", &CandleFeature::getOneShot)
        .def_property_readonly("hardware_timestamp", &CandleFeature::getHardwareTimestamp)
        .def_property_readonly("pad_package", &CandleFeature::getPadPackage)
        .def_property_readonly("fd", &CandleFeature::getFD)
        .def_property_readonly("bit_error_reporting", &CandleFeature::getBitErrorReporting)
        .def_property_readonly("termination", &CandleFeature::getTermination)
        .def_property_readonly("get_state", &CandleFeature::getGetState);

    py::class_<CandleBitTimingConst>(m, "CandleBitTimingConst")
        .def_property_readonly("tseg1_min", &CandleBitTimingConst::getTseg1Min)
        .def_property_readonly("tseg1_max", &CandleBitTimingConst::getTseg1Max)
        .def_property_readonly("tseg2_min", &CandleBitTimingConst::getTseg2Min)
        .def_property_readonly("tseg2_max", &CandleBitTimingConst::getTseg2Max)
        .def_property_readonly("sjw_max", &CandleBitTimingConst::getSjwMax)
        .def_property_readonly("brp_min", &CandleBitTimingConst::getBrpMin)
        .def_property_readonly("brp_max", &CandleBitTimingConst::getBrpMax)
        .def_property_readonly("brp_inc", &CandleBitTimingConst::getBrpInc);

    py::class_<CandleState>(m, "CandleState")
        .def_property_readonly("state", &CandleState::getState)
        .def_property_readonly("rx_error_count", &CandleState::getRxErrorCount)
        .def_property_readonly("tx_error_count", &CandleState::getTxErrorCount);

    py::class_<CandleCanState>(m, "CandleCanState")
        .def_property_readonly("error_active", &CandleCanState::getErrorActivate)
        .def_property_readonly("error_warning", &CandleCanState::getErrorWarning)
        .def_property_readonly("error_passive", &CandleCanState::getErrorPassive)
        .def_property_readonly("bus_off", &CandleCanState::getBusOff)
        .def_property_readonly("stopped", &CandleCanState::getStopped)
        .def_property_readonly("sleeping", &CandleCanState::getSleeping);

    py::class_<CandleChannel>(m, "CandleChannel")
        .def_property_readonly("feature", &CandleChannel::getFeature)
        .def_property_readonly("clock_frequency", &CandleChannel::getClockFrequency)
        .def_property_readonly("nominal_bit_timing_const", &CandleChannel::getNominalBitTimingConst)
        .def_property_readonly("data_bit_timing_const", &CandleChannel::getDataBitTimingConst)
        .def_property_readonly("state", &CandleChannel::getState)
        .def_property_readonly("termination", &CandleChannel::getTermination)
        .def("reset", &CandleChannel::reset)
        .def("start", &CandleChannel::start, py::arg("listen_only") = false, py::arg("loop_back") = false, py::arg("triple_sample") = false, py::arg("one_shot") = false, py::arg("hardware_timestamp") = false, py::arg("pad_package") = false, py::arg("fd") = false, py::arg("bit_error_reporting") = false)
        .def("set_bit_timing", &CandleChannel::setBitTiming)
        .def("set_data_bit_timing", &CandleChannel::setDataBitTiming)
        .def("set_termination", &CandleChannel::setTermination)
        .def("send_nowait", &CandleChannel::sendNowait)
        .def("receive_nowait", &CandleChannel::receiveNowait)
        .def("send", &CandleChannel::send)
        .def("receive", &CandleChannel::receive);

    py::class_<CandleDevice>(m, "CandleDevice")
        .def_property_readonly("is_connected", &CandleDevice::getIsConnected)
        .def_property_readonly("is_open", &CandleDevice::getIsOpen)
        .def_property_readonly("vendor_id", &CandleDevice::getVendorId)
        .def_property_readonly("product_id", &CandleDevice::getProductId)
        .def_property_readonly("manufacturer", &CandleDevice::getManufacturer)
        .def_property_readonly("product", &CandleDevice::getProduct)
        .def_property_readonly("serial_number", &CandleDevice::getSerialNumber)
        .def_property_readonly("channel_count", &CandleDevice::getChannelCount)
        .def_property_readonly("software_version", &CandleDevice::getSoftwareVersion)
        .def_property_readonly("hardware_version", &CandleDevice::getHardwareVersion)
        .def("open", &CandleDevice::open)
        .def("close", &CandleDevice::close)
        .def("__getitem__", &CandleDevice::getChannel)
        .def("__len__", &CandleDevice::getChannelCount);

    m.def("list_device", list_device);
}
