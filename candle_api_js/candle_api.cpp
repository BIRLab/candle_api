#include <napi.h>
#include <candle_api.h>

static const uint8_t dlc2len[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};

class CandleJS : public Napi::Addon<CandleJS> {
public:
    CandleJS(Napi::Env env, Napi::Object exports) {
        candle_initialize();
        DefineAddon(exports, {
            InstanceMethod("listDevice", &CandleJS::ListDevice, napi_enumerable),
            InstanceMethod("isOpened", &CandleJS::IsOpened, napi_enumerable),
            InstanceMethod("openDevice", &CandleJS::OpenDevice, napi_enumerable),
            InstanceMethod("closeDevice", &CandleJS::CloseDevice, napi_enumerable),
            InstanceMethod("getChannelInfo", &CandleJS::GetChannelInfo, napi_enumerable),
            InstanceMethod("resetChannel", &CandleJS::ResetChannel, napi_enumerable),
            InstanceMethod("startChannel", &CandleJS::StartChannel, napi_enumerable),
            InstanceMethod("setBitTiming", &CandleJS::SetBitTiming, napi_enumerable),
            InstanceMethod("setDataBitTiming", &CandleJS::SetDataBitTiming, napi_enumerable),
            InstanceMethod("setTermination", &CandleJS::SetTermination, napi_enumerable),
            InstanceMethod("send", &CandleJS::Send, napi_enumerable),
            InstanceMethod("receive", &CandleJS::Receive, napi_enumerable),
            InstanceValue("FRAME_TYPE_RX", Napi::Number::New(env, CANDLE_FRAME_TYPE_RX), napi_enumerable),
            InstanceValue("FRAME_TYPE_EFF", Napi::Number::New(env, CANDLE_FRAME_TYPE_EFF), napi_enumerable),
            InstanceValue("FRAME_TYPE_RTR", Napi::Number::New(env, CANDLE_FRAME_TYPE_RTR), napi_enumerable),
            InstanceValue("FRAME_TYPE_ERR", Napi::Number::New(env, CANDLE_FRAME_TYPE_ERR), napi_enumerable),
            InstanceValue("FRAME_TYPE_FD", Napi::Number::New(env, CANDLE_FRAME_TYPE_FD), napi_enumerable),
            InstanceValue("FRAME_TYPE_BRS", Napi::Number::New(env, CANDLE_FRAME_TYPE_BRS), napi_enumerable),
            InstanceValue("FRAME_TYPE_ESI", Napi::Number::New(env, CANDLE_FRAME_TYPE_ESI), napi_enumerable),
        });
    }

    ~CandleJS() {
        candle_finalize();
    }

private:
    Napi::Value ListDevice(const Napi::CallbackInfo& info) {
        candle_device **device_list;
        size_t device_list_size;
        if (!candle_get_device_list(&device_list, &device_list_size)) {
            throw Napi::Error::New(info.Env(), "Cannot get candle device list");
        }

        Napi::Array list = Napi::Array::New(info.Env(), device_list_size);
        for (int i = 0; i < device_list_size; ++i) {
            candle_device *handle = device_list[i];

            Napi::Object obj = Napi::Object::New(info.Env());
            obj.Set("vendor_id", handle->vendor_id);
            obj.Set("product_id", handle->product_id);
            obj.Set("manufacturer", handle->manufacturer);
            obj.Set("product", handle->product);
            obj.Set("serial_number", handle->serial_number);

            candle_ref_device(handle);
            obj.Set("handle", Napi::External<candle_device>::New(info.Env(), handle, [](Napi::BasicEnv e, candle_device* h){
                candle_unref_device(h);
            }));
            list[i] = obj;
        }

        candle_free_device_list(device_list);
        return list;
    }

    static candle_device* GetHandle(const Napi::CallbackInfo& info) {
        if (!info[0].IsObject())
            throw Napi::Error::New(info.Env(), "The first argument must be an object");

        if (!info[0].As<Napi::Object>().Has("handle"))
            throw Napi::Error::New(info.Env(), "The object does not have a native device handle");

        if (!info[0].As<Napi::Object>().Get("handle").IsExternal())
            throw Napi::Error::New(info.Env(), "The handle is not a external object");

        return info[0].As<Napi::Object>().Get("handle").As<Napi::External<candle_device>>().Data();
    }

    Napi::Value IsOpened(const Napi::CallbackInfo& info) {
        if (info.Length() != 1) {
            throw Napi::Error::New(info.Env(), "Wrong number of arguments");
        }

        candle_device *handle = GetHandle(info);

        return Napi::Boolean::New(info.Env(), handle->is_open && handle->is_connected);
    }

    Napi::Value OpenDevice(const Napi::CallbackInfo& info) {
        if (info.Length() != 1) {
            throw Napi::Error::New(info.Env(), "Wrong number of arguments");
        }

        candle_device *handle = GetHandle(info);

        if (candle_open_device(handle)) {
            candle_unref_device(handle);
            return Napi::Boolean::New(info.Env(), true);
        }

        return Napi::Boolean::New(info.Env(), false);
    }

    Napi::Value CloseDevice(const Napi::CallbackInfo& info) {
        if (info.Length() != 1) {
            throw Napi::Error::New(info.Env(), "Wrong number of arguments");
        }

        candle_device *handle = GetHandle(info);

        if (handle->is_open) {
            candle_ref_device(handle);
            candle_close_device(handle);
        }

        return info.Env().Null();
    }

    Napi::Object BitTimingConstToObject(Napi::Env env, const candle_bit_timing_const& bt) {
        Napi::Object obj = Napi::Object::New(env);
        obj.Set("tseg1_min", bt.tseg1_min);
        obj.Set("tseg1_max", bt.tseg1_max);
        obj.Set("tseg2_min", bt.tseg2_min);
        obj.Set("tseg2_max", bt.tseg2_max);
        obj.Set("sjw_max", bt.sjw_max);
        obj.Set("brp_min", bt.brp_min);
        obj.Set("brp_max", bt.brp_max);
        obj.Set("brp_inc", bt.brp_inc);
        return obj;
    }

    Napi::Value GetChannelInfo(const Napi::CallbackInfo& info) {
        if (info.Length() != 2) {
            throw Napi::Error::New(info.Env(), "Wrong number of arguments");
        }

        candle_device *handle = GetHandle(info);
        uint8_t channel = info[1].As<Napi::Number>().Uint32Value();

        Napi::Object obj = Napi::Object::New(info.Env());
        obj.Set("feature", uint32_t(handle->channels[channel].feature));
        obj.Set("clock_frequency", handle->channels[channel].clock_frequency);

        Napi::Object bt_const = Napi::Object::New(info.Env());
        bt_const.Set("nominal", BitTimingConstToObject(info.Env(), handle->channels[channel].bit_timing_const.nominal));
        bt_const.Set("data", BitTimingConstToObject(info.Env(), handle->channels[channel].bit_timing_const.data));
        obj.Set("bit_timing_const", bt_const);

        return obj;
    }

    Napi::Value ResetChannel(const Napi::CallbackInfo& info) {
        if (info.Length() != 2) {
            throw Napi::Error::New(info.Env(), "Wrong number of arguments");
        }

        candle_device *handle = GetHandle(info);
        uint8_t channel = info[1].As<Napi::Number>().Uint32Value();

        return Napi::Boolean::New(info.Env(), candle_reset_channel(handle, channel));
    }

    Napi::Value StartChannel(const Napi::CallbackInfo& info) {
        if (info.Length() != 3) {
            throw Napi::Error::New(info.Env(), "Wrong number of arguments");
        }

        candle_device *handle = GetHandle(info);
        uint8_t channel = info[1].As<Napi::Number>().Uint32Value();
        uint32_t mode = info[2].As<Napi::Number>().Uint32Value();

        if (handle->channels[channel].feature & CANDLE_FEATURE_HW_TIMESTAMP)
            mode |= CANDLE_MODE_HW_TIMESTAMP;

        return Napi::Boolean::New(info.Env(), candle_start_channel(handle, channel, static_cast<candle_mode>(mode)));
    }

    static void GetBitTiming(const Napi::Object& obj, candle_bit_timing& bt) {
        bt.prop_seg = obj.Get("prop_seg").As<Napi::Number>().Uint32Value();
        bt.phase_seg1 = obj.Get("phase_seg1").As<Napi::Number>().Uint32Value();
        bt.phase_seg2 = obj.Get("phase_seg2").As<Napi::Number>().Uint32Value();
        bt.sjw = obj.Get("sjw").As<Napi::Number>().Uint32Value();
        bt.brp = obj.Get("brp").As<Napi::Number>().Uint32Value();
    }

    Napi::Value SetBitTiming(const Napi::CallbackInfo& info) {
        if (info.Length() != 3) {
            throw Napi::Error::New(info.Env(), "Wrong number of arguments");
        }

        candle_device *handle = GetHandle(info);
        uint8_t channel = info[1].As<Napi::Number>().Uint32Value();

        candle_bit_timing bt;
        GetBitTiming(info[2].As<Napi::Object>(), bt);

        return Napi::Boolean::New(info.Env(), candle_set_bit_timing(handle, channel, &bt));
    }

    Napi::Value SetDataBitTiming(const Napi::CallbackInfo& info) {
        if (info.Length() != 3) {
            throw Napi::Error::New(info.Env(), "Wrong number of arguments");
        }

        candle_device *handle = GetHandle(info);
        uint8_t channel = info[1].As<Napi::Number>().Uint32Value();

        candle_bit_timing bt;
        GetBitTiming(info[2].As<Napi::Object>(), bt);

        return Napi::Boolean::New(info.Env(), candle_set_data_bit_timing(handle, channel, &bt));
    }

    Napi::Value SetTermination(const Napi::CallbackInfo& info) {
        if (info.Length() != 3) {
            throw Napi::Error::New(info.Env(), "Wrong number of arguments");
        }

        candle_device *handle = GetHandle(info);
        uint8_t channel = info[1].As<Napi::Number>().Uint32Value();
        bool enable = info[2].As<Napi::Boolean>().Value();

        return Napi::Boolean::New(info.Env(), candle_set_termination(handle, channel, enable));
    }

    static Napi::Object FrameToObject(const Napi::Env& env, const candle_can_frame& frame) {
        Napi::Number type = Napi::Number::New(env, frame.type);
        Napi::Number can_id = Napi::Number::New(env, frame.can_id);
        Napi::Number can_dlc = Napi::Number::New(env, frame.can_dlc);
        Napi::Array data = Napi::Array::New(env, dlc2len[frame.can_dlc]);
        Napi::Number timestamp_us = Napi::Number::New(env, frame.timestamp_us);

        for (int i = 0; i < dlc2len[frame.can_dlc]; ++i) {
            data[i] = Napi::Number::New(env, frame.data[i]);
        }

        Napi::Object obj = Napi::Object::New(env);
        obj.Set("type", type);
        obj.Set("can_id", can_id);
        obj.Set("can_dlc", can_dlc);
        obj.Set("data", data);
        obj.Set("timestamp_us", timestamp_us);
        return obj;
    }

    static void ObjectToFrame(const Napi::Object& obj, candle_can_frame& frame) {
        frame.type = static_cast<candle_frame_type>(obj.Get("type").As<Napi::Number>().Uint32Value());
        frame.can_id = obj.Get("can_id").As<Napi::Number>().Uint32Value();
        frame.can_dlc = obj.Get("can_dlc").As<Napi::Number>().Uint32Value();
        for (int i = 0; i < dlc2len[frame.can_dlc]; ++i) {
            frame.data[i] = obj.Get("data").As<Napi::Array>().Get(i).As<Napi::Number>().Uint32Value();
        }
        frame.timestamp_us = 0;
    }

    template<bool (*send_receive_function)(candle_device *device, uint8_t channel, candle_can_frame *frame, uint32_t milliseconds)>
    class SendReceiveWorker : public Napi::AsyncWorker {
    public:
        SendReceiveWorker(Napi::Env env, Napi::Promise::Deferred deferred, candle_device *dev, uint8_t channel, uint32_t timeout) : AsyncWorker(env), _deferred{deferred}, _dev{dev}, _channel{channel}, _timeout{timeout}, _frame{}, _result{false} {}
        SendReceiveWorker(Napi::Env env, Napi::Promise::Deferred deferred, candle_device *dev, uint8_t channel, uint32_t timeout, const struct candle_can_frame& frame) : AsyncWorker(env), _deferred{deferred}, _dev{dev}, _channel{channel}, _timeout{timeout}, _frame{frame}, _result{false} {}

        void Execute() override {
            _result = send_receive_function(_dev, _channel, &_frame, _timeout);
        }

        void OnOK() override {
            Napi::HandleScope scope(Env());
            if (_result)
                _deferred.Resolve(FrameToObject(Env(), _frame));
            else
                _deferred.Reject({Napi::String::New(Env(), "Timeout")});
        }

    private:
        Napi::Promise::Deferred _deferred;
        candle_device *_dev;
        uint8_t _channel;
        uint32_t _timeout;
        candle_can_frame _frame;
        bool _result;
    };

    Napi::Value Send(const Napi::CallbackInfo& info) {
        if (info.Length() != 4) {
            throw Napi::Error::New(info.Env(), "Wrong number of arguments");
        }

        candle_device *handle = GetHandle(info);
        uint8_t channel = info[1].As<Napi::Number>().Uint32Value();
        uint32_t timeout = info[2].As<Napi::Number>().Uint32Value();

        candle_can_frame frame{};
        ObjectToFrame(info[3].As<Napi::Object>(), frame);
        Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
        SendReceiveWorker<candle_send_frame>* wk = new SendReceiveWorker<candle_send_frame>(info.Env(), deferred, handle, channel, timeout, frame);
        wk->Queue();
        return deferred.Promise();
    }

    Napi::Value Receive(const Napi::CallbackInfo& info) {
        if (info.Length() != 3) {
            throw Napi::Error::New(info.Env(), "Wrong number of arguments");
        }

        candle_device *handle = GetHandle(info);
        uint8_t channel = info[1].As<Napi::Number>().Uint32Value();
        uint32_t timeout = info[2].As<Napi::Number>().Uint32Value();

        Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
        SendReceiveWorker<candle_receive_frame>* wk = new SendReceiveWorker<candle_receive_frame>(info.Env(), deferred, handle, channel, timeout);
        wk->Queue();
        return deferred.Promise();
    }
};

NODE_API_ADDON(CandleJS)