#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "candle_api.h"

/* CandleBitTimingConst */

typedef struct {
    PyObject_HEAD
    struct candle_bit_timing_const bt_const;
} CandleBitTimingConst_object;

static PyObject *CandleChannel_get_tseg1_min_property(PyObject *self, void *closure) {
    CandleBitTimingConst_object *obj = (CandleBitTimingConst_object *)self;
    return PyLong_FromLong((long)obj->bt_const.tseg1_min);
}

static PyObject *CandleChannel_get_tseg1_max_property(PyObject *self, void *closure) {
    CandleBitTimingConst_object *obj = (CandleBitTimingConst_object *)self;
    return PyLong_FromLong((long)obj->bt_const.tseg1_max);
}

static PyObject *CandleChannel_get_tseg2_min_property(PyObject *self, void *closure) {
    CandleBitTimingConst_object *obj = (CandleBitTimingConst_object *)self;
    return PyLong_FromLong((long)obj->bt_const.tseg2_min);
}

static PyObject *CandleChannel_get_tseg2_max_property(PyObject *self, void *closure) {
    CandleBitTimingConst_object *obj = (CandleBitTimingConst_object *)self;
    return PyLong_FromLong((long)obj->bt_const.tseg2_max);
}

static PyObject *CandleChannel_get_sjw_max_property(PyObject *self, void *closure) {
    CandleBitTimingConst_object *obj = (CandleBitTimingConst_object *)self;
    return PyLong_FromLong((long)obj->bt_const.sjw_max);
}

static PyObject *CandleChannel_get_brp_min_property(PyObject *self, void *closure) {
    CandleBitTimingConst_object *obj = (CandleBitTimingConst_object *)self;
    return PyLong_FromLong((long)obj->bt_const.brp_min);
}

static PyObject *CandleChannel_get_brp_max_property(PyObject *self, void *closure) {
    CandleBitTimingConst_object *obj = (CandleBitTimingConst_object *)self;
    return PyLong_FromLong((long)obj->bt_const.brp_max);
}

static PyObject *CandleChannel_get_brp_inc_property(PyObject *self, void *closure) {
    CandleBitTimingConst_object *obj = (CandleBitTimingConst_object *)self;
    return PyLong_FromLong((long)obj->bt_const.brp_inc);
}

static PyGetSetDef CandleBitTimingConst_getset[] = {
    {
        .name = "tseg1_min",
        .get = CandleChannel_get_tseg1_min_property
    },
    {
        .name = "tseg1_max",
        .get = CandleChannel_get_tseg1_max_property
    },
    {
        .name = "tseg2_min",
        .get = CandleChannel_get_tseg2_min_property
    },
    {
        .name = "tseg2_max",
        .get = CandleChannel_get_tseg2_max_property
    },
    {
        .name = "sjw_max",
        .get = CandleChannel_get_sjw_max_property
    },
    {
        .name = "brp_min",
        .get = CandleChannel_get_brp_min_property
    },
    {
        .name = "brp_max",
        .get = CandleChannel_get_brp_max_property
    },
    {
        .name = "brp_inc",
        .get = CandleChannel_get_brp_inc_property
    },
    {NULL}
};

static PyTypeObject CandleBitTimingConstType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "candle_api.CandleBitTimingConst",
    .tp_basicsize = sizeof(CandleBitTimingConst_object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_getset = CandleBitTimingConst_getset
};

/* CandleChannel */

typedef struct {
    PyObject_HEAD
    struct candle_device *dev;
    uint8_t ch;
} CandleChannel_object;

static void CandleChannel_dealloc(PyObject *self) {
    CandleChannel_object *obj = (CandleChannel_object *)self;

    candle_unref_device(obj->dev);

    Py_TYPE(self)->tp_free(self);
}

static PyObject *CandleChannel_get_feature_property(PyObject *self, void *closure) {
    CandleChannel_object *obj = (CandleChannel_object *)self;
    return PyLong_FromLong(obj->dev->channels[obj->ch].feature);
}

static PyObject *CandleChannel_get_clock_frequency_property(PyObject *self, void *closure) {
    CandleChannel_object *obj = (CandleChannel_object *)self;
    return PyLong_FromLong((long)obj->dev->channels[obj->ch].clock_frequency);
}

static PyObject *CandleChannel_get_nominal_bit_timing_property(PyObject *self, void *closure) {
    CandleChannel_object *obj = (CandleChannel_object *)self;

    CandleBitTimingConst_object *bt_obj = (CandleBitTimingConst_object*)PyObject_New(CandleBitTimingConst_object, &CandleBitTimingConstType);
    if (bt_obj == NULL)
        return NULL;

    bt_obj->bt_const = obj->dev->channels[obj->ch].bit_timing_const.nominal;

    return (PyObject*)bt_obj;
}

static PyObject *CandleChannel_get_data_bit_timing_property(PyObject *self, void *closure) {
    CandleChannel_object *obj = (CandleChannel_object *)self;

    CandleBitTimingConst_object *bt_obj = (CandleBitTimingConst_object*)PyObject_New(CandleBitTimingConst_object, &CandleBitTimingConstType);
    if (bt_obj == NULL)
        return NULL;

    bt_obj->bt_const = obj->dev->channels[obj->ch].bit_timing_const.data;

    return (PyObject*)bt_obj;
}

static PyGetSetDef CandleChannel_getset[] = {
    {
        .name = "feature",
        .get = CandleChannel_get_feature_property
    },
    {
        .name = "clock_frequency",
        .get = CandleChannel_get_clock_frequency_property
    },
    {
        .name = "nominal_bit_timing",
        .get = CandleChannel_get_nominal_bit_timing_property
    },
    {
        .name = "data_bit_timing",
        .get = CandleChannel_get_data_bit_timing_property
    },
    {NULL}
};

static PyTypeObject CandleChannelType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "candle_api.CandleChannel",
    .tp_basicsize = sizeof(CandleChannel_object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_dealloc = CandleChannel_dealloc,
    .tp_getset = CandleChannel_getset
};

/* CandleDevice */

typedef struct {
    PyObject_HEAD
    struct candle_device *dev;
} CandleDevice_object;

static void CandleDevice_dealloc(PyObject *self) {
    CandleDevice_object *obj = (CandleDevice_object *)self;

    candle_close_device(obj->dev);
    candle_unref_device(obj->dev);

    Py_TYPE(self)->tp_free(self);
}

static PyObject *CandleDevice_get_is_connected_property(PyObject *self, void *closure) {
    CandleDevice_object *obj = (CandleDevice_object *)self;

    if (obj->dev->is_connected)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyObject *CandleDevice_get_is_open_property(PyObject *self, void *closure) {
    CandleDevice_object *obj = (CandleDevice_object *)self;

    if (obj->dev->is_open)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyObject *CandleDevice_get_vendor_id_property(PyObject *self, void *closure) {
    CandleDevice_object *obj = (CandleDevice_object *)self;
    return PyLong_FromLong(obj->dev->vendor_id);
}

static PyObject *CandleDevice_get_product_id_property(PyObject *self, void *closure) {
    CandleDevice_object *obj = (CandleDevice_object *)self;
    return PyLong_FromLong(obj->dev->product_id);
}

static PyObject *CandleDevice_get_manufacturer_property(PyObject *self, void *closure) {
    CandleDevice_object *obj = (CandleDevice_object *)self;
    return PyUnicode_FromString(obj->dev->manufacturer);
}

static PyObject *CandleDevice_get_product_property(PyObject *self, void *closure) {
    CandleDevice_object *obj = (CandleDevice_object *)self;
    return PyUnicode_FromString(obj->dev->product);
}

static PyObject *CandleDevice_get_serial_number_property(PyObject *self, void *closure) {
    CandleDevice_object *obj = (CandleDevice_object *)self;
    return PyUnicode_FromString(obj->dev->serial_number);
}

static PyObject *CandleDevice_get_channel_count_property(PyObject *self, void *closure) {
    CandleDevice_object *obj = (CandleDevice_object *)self;
    return PyLong_FromLong(obj->dev->channel_count);
}

static PyObject *CandleDevice_get_software_version_property(PyObject *self, void *closure) {
    CandleDevice_object *obj = (CandleDevice_object *)self;
    return PyLong_FromLong((long)obj->dev->software_version);
}

static PyObject *CandleDevice_get_hardware_version_property(PyObject *self, void *closure) {
    CandleDevice_object *obj = (CandleDevice_object *)self;
    return PyLong_FromLong((long)obj->dev->hardware_version);
}

static PyGetSetDef CandleDevice_getset[] = {
    {
        .name = "is_connected",
        .get = CandleDevice_get_is_connected_property
    },
    {
        .name = "is_open",
        .get = CandleDevice_get_is_open_property
    },
    {
        .name = "vendor_id",
        .get = CandleDevice_get_vendor_id_property
    },
    {
        .name = "product_id",
        .get = CandleDevice_get_product_id_property
    },
    {
        .name = "manufacturer",
        .get = CandleDevice_get_manufacturer_property
    },
    {
        .name = "product",
        .get = CandleDevice_get_product_property
    },
    {
        .name = "serial_number",
        .get = CandleDevice_get_serial_number_property
    },
    {
        .name = "channel_count",
        .get = CandleDevice_get_channel_count_property
    },
    {
        .name = "software_version",
        .get = CandleDevice_get_software_version_property
    },
    {
        .name = "hardware_version",
        .get = CandleDevice_get_hardware_version_property
    },
    {NULL}
};

static Py_ssize_t CandleDevice_length(PyObject *self) {
    CandleDevice_object *obj = (CandleDevice_object *)self;
    return obj->dev->channel_count;
}

static PyObject *CandleDevice_getitem(PyObject *self, Py_ssize_t idx) {
    CandleDevice_object *obj = (CandleDevice_object *)self;

    if (idx < 0 || idx >= obj->dev->channel_count) {
        PyErr_SetString(PyExc_IndexError, "Index out of range");
        return NULL;
    }

    CandleChannel_object* ch_obj = (CandleChannel_object*)PyObject_New(CandleChannel_object, &CandleChannelType);
    if (ch_obj == NULL)
        return NULL;

    ch_obj->dev = obj->dev;
    ch_obj->ch = (uint8_t)idx;
    candle_ref_device(ch_obj->dev);

    return (PyObject *)ch_obj;
}

static PySequenceMethods CandleDevice_as_sequence = {
    .sq_length = CandleDevice_length,
    .sq_item = CandleDevice_getitem,
};

static PyObject *CandleDevice_open(CandleDevice_object *self, PyObject *Py_UNUSED(ignored))
{
    if (candle_open_device(self->dev))
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject *CandleDevice_close(CandleDevice_object *self, PyObject *Py_UNUSED(ignored))
{
    candle_close_device(self->dev);
    Py_RETURN_NONE;
}

static PyMethodDef CandleDevice_methods[] = {
    {
        .ml_name = "open",
        .ml_meth = (PyCFunction)CandleDevice_open,
        .ml_flags = METH_NOARGS
    },
    {
        .ml_name = "close",
        .ml_meth = (PyCFunction)CandleDevice_close,
        .ml_flags = METH_NOARGS
    },
    {NULL}
};

static PyTypeObject CandleDeviceType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "candle_api.CandleDevice",
    .tp_basicsize = sizeof(CandleDevice_object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_dealloc = CandleDevice_dealloc,
    .tp_getset = CandleDevice_getset,
    .tp_as_sequence = &CandleDevice_as_sequence,
    .tp_methods = CandleDevice_methods
};

/* candle_api */

static PyObject *list_device(PyObject *self, PyObject *args)
{
    struct candle_device **device_list;
    size_t device_list_size;
    if (!candle_get_device_list(&device_list, &device_list_size))
        return NULL;

    PyObject* list = PyList_New((long)device_list_size);
    if (list == NULL) {
        return NULL;
    }

    for (int i = 0; i < device_list_size; ++i) {
        CandleDevice_object* obj = (CandleDevice_object*)PyObject_New(CandleDevice_object, &CandleDeviceType);
        if (obj == NULL) {
            Py_DECREF(list);
            return NULL;
        }
        obj->dev = device_list[i];
        candle_ref_device(obj->dev);
        PyList_SetItem(list, i, (PyObject *)obj);
    }

    candle_free_device_list(device_list);

    return list;
}

static PyMethodDef candle_api_methods[] = {
        {"list_device", list_device, METH_NOARGS, "List all connected gs_usb devices."},
        {NULL, NULL, 0, NULL}
};

void cleanup(void *module) {
    candle_finalize();
}

static struct PyModuleDef candle_api_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "candle_api",
    .m_size = -1,
    .m_methods = candle_api_methods,
    .m_free = cleanup,
};

PyMODINIT_FUNC PyInit_candle_api(void)
{
    PyObject *m;

    if (candle_initialize() != true)
        return NULL;

    if (PyType_Ready(&CandleBitTimingConstType) < 0)
        return NULL;

    if (PyType_Ready(&CandleChannelType) < 0)
        return NULL;

    if (PyType_Ready(&CandleDeviceType) < 0)
        return NULL;

    m = PyModule_Create(&candle_api_module);
    if (m == NULL)
        return NULL;

    Py_INCREF(&CandleBitTimingConstType);
    if (PyModule_AddObject(m, "CandleBitTimingConstType", (PyObject *)&CandleBitTimingConstType) < 0) {
        Py_DECREF(&CandleBitTimingConstType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&CandleChannelType);
    if (PyModule_AddObject(m, "CandleChannel", (PyObject *)&CandleChannelType) < 0) {
        Py_DECREF(&CandleChannelType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&CandleDeviceType);
    if (PyModule_AddObject(m, "CandleDevice", (PyObject *)&CandleDeviceType) < 0) {
        Py_DECREF(&CandleDeviceType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
