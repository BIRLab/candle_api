#include <napi.h>
#include <candle_api.h>

static const uint8_t dlc2len[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};

class CandleDevice : public Napi::ObjectWrap<CandleDevice> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "CandleDevice", {
            InstanceAccessor<&CandleDevice::VIDGetter>("idVender"),
            InstanceAccessor<&CandleDevice::PIDGetter>("idProduct"),
            InstanceAccessor<&CandleDevice::ManufactureGetter>("manufacture"),
            InstanceAccessor<&CandleDevice::ProductGetter>("product"),
            InstanceAccessor<&CandleDevice::SerialNumberGetter>("serialNumber"),
            InstanceMethod<&CandleDevice::Open>("open"),
            InstanceMethod<&CandleDevice::Close>("close"),
            InstanceMethod<&CandleDevice::Send>("send"),
            InstanceMethod<&CandleDevice::Receive>("receive"),
        });
        Napi::FunctionReference* constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        exports.Set("CandleDevice", func);
        env.SetInstanceData<Napi::FunctionReference>(constructor);
        return exports;
    }

    CandleDevice(const Napi::CallbackInfo& info) : Napi::ObjectWrap<CandleDevice>(info) {
        if (info.Length() != 1) {
            Napi::TypeError::New(info.Env(), "Wrong number of arguments").ThrowAsJavaScriptException();
            return;
        }

        if (!info[0].IsExternal()) {
            Napi::TypeError::New(info.Env(), "Need an external argument").ThrowAsJavaScriptException();
            return;
        }

        _dev = info[0].As<Napi::External<struct candle_device>>().Data();
        candle_ref_device(_dev);
    }

    void Finalize(Napi::Env env) {
        candle_unref_device(_dev);
    }

    static Napi::Value Create(Napi::Env env, struct candle_device *dev)  {
        Napi::FunctionReference* constructor = env.GetInstanceData<Napi::FunctionReference>();
        return constructor->New({ Napi::External<struct candle_device>::New(env, dev) });
    }

private:
    struct candle_device *_dev;

    class SendWorker : public Napi::AsyncWorker {
    public:
        SendWorker(Napi::Env env, Napi::Promise::Deferred deferred, struct candle_device *dev, const struct candle_can_frame& frame) : AsyncWorker(env), _deferred{deferred}, _dev{dev}, _frame{frame} {}

        void Execute() override {
            _result = candle_send_frame(_dev, 0, &_frame, 1000);
        }

        void OnOK() override {
            Napi::HandleScope scope(Env());
            if (_result)
                _deferred.Resolve({Napi::Boolean::New(Env(), _result)});
            else
                _deferred.Reject({Napi::String::New(Env(), "Error!!!!!!")});
        }
    private:
        Napi::Promise::Deferred _deferred;
        struct candle_device *_dev;
        struct candle_can_frame _frame;
        bool _result{};
    };

    class ReceiveWorker : public Napi::AsyncWorker {
    public:
        ReceiveWorker(Napi::Env env, Napi::Promise::Deferred deferred, struct candle_device *dev) : AsyncWorker(env), _deferred{deferred}, _dev{dev} {}

        void Execute() override {
            _result = candle_receive_frame(_dev, 0, &_frame, 1000);
        }

        void OnOK() override {
            Napi::HandleScope scope(Env());
            if (_result) {
                Napi::Number can_id = Napi::Number::New(Env(), _frame.can_id);
                Napi::Number can_dlc = Napi::Number::New(Env(), _frame.can_dlc);
                Napi::Array data = Napi::Array::New(Env(), dlc2len[_frame.can_dlc]);
                for (int i = 0; i < dlc2len[_frame.can_dlc]; ++i) {
                    data[i] = Napi::Number::New(Env(), _frame.data[i]);
                }
                Napi::Object obj = Napi::Object::New(Env());
                obj.Set("id", can_id);
                obj.Set("dlc", can_dlc);
                obj.Set("data", data);
                _deferred.Resolve(obj);
            } else {
                _deferred.Reject(Napi::String::New(Env(), "Error!!!!!!"));
            }
        }
    private:
        Napi::Promise::Deferred _deferred;
        struct candle_device *_dev;
        struct candle_can_frame _frame{};
        bool _result{};
    };

    Napi::Value VIDGetter(const Napi::CallbackInfo& info) {
        return Napi::Number::New(info.Env(), _dev->vendor_id);
    }

    Napi::Value PIDGetter(const Napi::CallbackInfo& info) {
        return Napi::Number::New(info.Env(), _dev->product_id);
    }

    Napi::Value ManufactureGetter(const Napi::CallbackInfo& info) {
        return Napi::String::New(info.Env(), _dev->manufacturer);
    }

    Napi::Value ProductGetter(const Napi::CallbackInfo& info) {
        return Napi::String::New(info.Env(), _dev->product);
    }

    Napi::Value SerialNumberGetter(const Napi::CallbackInfo& info) {
        return Napi::String::New(info.Env(), _dev->serial_number);
    }

    Napi::Value Open(const Napi::CallbackInfo& info) {
        if (info.Length() != 1) {
            Napi::TypeError::New(info.Env(), "Wrong number of arguments").ThrowAsJavaScriptException();
            return Napi::Boolean::New(info.Env(), false);
        }

        if (!info[0].IsObject()) {
            Napi::TypeError::New(info.Env(), "Need an object argument").ThrowAsJavaScriptException();
            return Napi::Boolean::New(info.Env(), false);
        }

        struct candle_bit_timing bt;
        bt.prop_seg = info[0].As<Napi::Object>().Get("bit_timing").As<Napi::Object>().Get("prop_seg").As<Napi::Number>().operator uint32_t();
        bt.phase_seg1 = info[0].As<Napi::Object>().Get("bit_timing").As<Napi::Object>().Get("phase_seg1").As<Napi::Number>().operator uint32_t();
        bt.phase_seg2 = info[0].As<Napi::Object>().Get("bit_timing").As<Napi::Object>().Get("phase_seg2").As<Napi::Number>().operator uint32_t();
        bt.sjw = info[0].As<Napi::Object>().Get("bit_timing").As<Napi::Object>().Get("sjw").As<Napi::Number>().operator uint32_t();
        bt.brp = info[0].As<Napi::Object>().Get("bit_timing").As<Napi::Object>().Get("brp").As<Napi::Number>().operator uint32_t();

        struct candle_bit_timing dbt;
        dbt.prop_seg = info[0].As<Napi::Object>().Get("data_bit_timing").As<Napi::Object>().Get("prop_seg").As<Napi::Number>().operator uint32_t();
        dbt.phase_seg1 = info[0].As<Napi::Object>().Get("data_bit_timing").As<Napi::Object>().Get("phase_seg1").As<Napi::Number>().operator uint32_t();
        dbt.phase_seg2 = info[0].As<Napi::Object>().Get("data_bit_timing").As<Napi::Object>().Get("phase_seg2").As<Napi::Number>().operator uint32_t();
        dbt.sjw = info[0].As<Napi::Object>().Get("data_bit_timing").As<Napi::Object>().Get("sjw").As<Napi::Number>().operator uint32_t();
        dbt.brp = info[0].As<Napi::Object>().Get("data_bit_timing").As<Napi::Object>().Get("brp").As<Napi::Number>().operator uint32_t();

        if (!candle_open_device(_dev))
            return Napi::Boolean::New(info.Env(), false);

        candle_unref_device(_dev);

        if (!candle_reset_channel(_dev, 0))
            return Napi::Boolean::New(info.Env(), false);

        if (!candle_set_bit_timing(_dev, 0, &bt))
            return Napi::Boolean::New(info.Env(), false);

        if (!candle_set_data_bit_timing(_dev, 0, &dbt))
            return Napi::Boolean::New(info.Env(), false);

        candle_set_termination(_dev, 0, true);

        if (!candle_start_channel(_dev, 0, static_cast<candle_mode>(CANDLE_MODE_HW_TIMESTAMP | CANDLE_MODE_FD)))
            return Napi::Boolean::New(info.Env(), false);

        return Napi::Boolean::New(info.Env(), true);
    }

    Napi::Value Close(const Napi::CallbackInfo& info) {
        if (_dev->is_open) {
            candle_ref_device(_dev);
            candle_close_device(_dev);
        }
        return info.Env().Null();
    }

    Napi::Value Send(const Napi::CallbackInfo& info) {
        struct candle_can_frame frame;
        frame.type = static_cast<candle_frame_type>(CANDLE_FRAME_TYPE_FD | CANDLE_FRAME_TYPE_BRS);
        frame.can_id = info[0].As<Napi::Object>().Get("id").As<Napi::Number>().Uint32Value();
        frame.can_dlc = info[0].As<Napi::Object>().Get("dlc").As<Napi::Number>().Uint32Value();
        for (int i = 0; i < dlc2len[frame.can_dlc]; ++i) {
            frame.data[i] = info[0].As<Napi::Object>().Get("data").As<Napi::Array>().Get(i).As<Napi::Number>().Uint32Value();
        }
        frame.timestamp_us = 0;

        Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
        SendWorker* wk = new SendWorker(info.Env(), deferred, _dev, frame);
        wk->Queue();
        return deferred.Promise();
    }

    Napi::Value Receive(const Napi::CallbackInfo& info) {
        Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
        ReceiveWorker* wk = new ReceiveWorker(info.Env(), deferred, _dev);
        wk->Queue();
        return deferred.Promise();
    }
};

Napi::Value ListDevice(const Napi::CallbackInfo& info) {
    struct candle_device **device_list;
    size_t device_list_size;
    if (!candle_get_device_list(&device_list, &device_list_size)) {
        Napi::TypeError::New(info.Env(), "Cannot get candle device list").ThrowAsJavaScriptException();
        return info.Env().Null();
    }

    Napi::Array list = Napi::Array::New(info.Env(), device_list_size);

    for (int i = 0; i < device_list_size; ++i) {
        list[i] = CandleDevice::Create(info.Env(), device_list[i]);
    }

    candle_free_device_list(device_list);

    return list;
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    candle_initialize();
    exports.Set("listDevice", Napi::Function::New<ListDevice>(env));
    CandleDevice::Init(env, exports);
    return exports;
}

NODE_API_MODULE(candle-js, Init)