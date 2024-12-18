#include <napi.h>
#include <candle_api.h>

class CandleJSAddon : public Napi::Addon<CandleJSAddon> {
public:
    CandleJSAddon(Napi::Env env, Napi::Object exports) {
        candle_initialize();

        DefineAddon(exports, {
            InstanceMethod("listDevice", &CandleJSAddon::ListDevice)
        });
    }

    ~CandleJSAddon() {
        candle_finalize();
    }

private:
    Napi::Value ListDevice(const Napi::CallbackInfo& info) {
        struct candle_device **device_list;
        size_t device_list_size;
        if (!candle_get_device_list(&device_list, &device_list_size))
            throw Napi::Error::New(info.Env(), "Error");;

        auto list = Napi::Array::New(info.Env(), device_list_size);

        for (int i = 0; i < device_list_size; ++i) {
            auto obj = Napi::Object::New(info.Env());
            obj.Set("vendor_id", device_list[i]->vendor_id);
            obj.Set("product_id", device_list[i]->product_id);
            obj.Set("manufacturer", device_list[i]->manufacturer);
            obj.Set("product", device_list[i]->product);
            obj.Set("serial_number", device_list[i]->serial_number);
            list[i] = obj;
        }

        candle_free_device_list(device_list);

        return list;
    }
};

NODE_API_ADDON(CandleJSAddon)