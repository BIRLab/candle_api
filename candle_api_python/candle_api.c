#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "candle_api.h"

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

static PyGetSetDef CandleChannel_getset[] = {
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
    return PyLong_FromLong(obj->dev->software_version);
}

static PyObject *CandleDevice_get_hardware_version_property(PyObject *self, void *closure) {
    CandleDevice_object *obj = (CandleDevice_object *)self;
    return PyLong_FromLong(obj->dev->hardware_version);
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

static PyTypeObject CandleDeviceType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "candle_api.CandleDevice",
    .tp_basicsize = sizeof(CandleDevice_object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_dealloc = CandleDevice_dealloc,
    .tp_getset = CandleDevice_getset,
    .tp_as_sequence = &CandleDevice_as_sequence
};

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

    if (PyType_Ready(&CandleDeviceType) < 0)
        return NULL;

    if (PyType_Ready(&CandleChannelType) < 0)
        return NULL;

    m = PyModule_Create(&candle_api_module);
    if (m == NULL)
        return NULL;

    Py_INCREF(&CandleDeviceType);
    if (PyModule_AddObject(m, "CandleDevice", (PyObject *)&CandleDeviceType) < 0) {
        Py_DECREF(&CandleDeviceType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&CandleChannelType);
    if (PyModule_AddObject(m, "CandleChannel", (PyObject *)&CandleChannelType) < 0) {
        Py_DECREF(&CandleChannelType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
