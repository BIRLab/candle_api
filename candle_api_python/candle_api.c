#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "candle_api.h"

/* CandleMode */

typedef struct {
    PyObject_HEAD
    enum candle_mode mode;
} CandleMode_object;

static int CandleMode_init(CandleMode_object *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {
        "listen_only",
        "loop_back",
        "triple_sample",
        "one_shot",
        "hardware_timestamp",
        "pad_package",
        "fd",
        "bit_error_reporting",
        NULL
    };
    int parsed_args[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|pppppppp", kwlist,
                                     &parsed_args[0], &parsed_args[1], &parsed_args[2], &parsed_args[3],
                                     &parsed_args[4], &parsed_args[5], &parsed_args[6], &parsed_args[7]))
        return -1;

    self->mode = CANDLE_MODE_NORMAL;
    if (parsed_args[0])
        self->mode |= CANDLE_MODE_LISTEN_ONLY;
    if (parsed_args[1])
        self->mode |= CANDLE_MODE_LOOP_BACK;
    if (parsed_args[2])
        self->mode |= CANDLE_MODE_TRIPLE_SAMPLE;
    if (parsed_args[3])
        self->mode |= CANDLE_MODE_ONE_SHOT;
    if (parsed_args[4])
        self->mode |= CANDLE_MODE_HW_TIMESTAMP;
    if (parsed_args[5])
        self->mode |= CANDLE_MODE_PAD_PKTS_TO_MAX_PKT_SIZE;
    if (parsed_args[6])
        self->mode |= CANDLE_MODE_FD;
    if (parsed_args[7])
        self->mode |= CANDLE_MODE_BERR_REPORTING;

    return 0;
}

static PyTypeObject CandleModeType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "candle_api.CandleMode",
    .tp_basicsize = sizeof(CandleMode_object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc)CandleMode_init
};

/* CandleFrameType */

typedef struct {
    PyObject_HEAD
    enum candle_frame_type type;
} CandleFrameType_object;

static int CandleFrameType_init(CandleFrameType_object *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {
            "rx",
            "extended_id",
            "remote_frame",
            "error_frame",
            "fd",
            "bitrate_switch",
            "error_state_indicator",
            NULL
    };
    int parsed_args[8] = {0, 0, 0, 0, 0, 0, 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ppppppp", kwlist,
                                     &parsed_args[0], &parsed_args[1], &parsed_args[2], &parsed_args[3],
                                     &parsed_args[4], &parsed_args[5], &parsed_args[6]))
        return -1;

    self->type = 0;
    if (parsed_args[0])
        self->type |= CANDLE_FRAME_TYPE_RX;
    if (parsed_args[1])
        self->type |= CANDLE_FRAME_TYPE_EFF;
    if (parsed_args[2])
        self->type |= CANDLE_FRAME_TYPE_RTR;
    if (parsed_args[3])
        self->type |= CANDLE_FRAME_TYPE_ERR;
    if (parsed_args[4])
        self->type |= CANDLE_FRAME_TYPE_FD;
    if (parsed_args[5])
        self->type |= CANDLE_FRAME_TYPE_BRS;
    if (parsed_args[6])
        self->type |= CANDLE_FRAME_TYPE_ESI;

    return 0;
}

#define CandleFrameType_PROPERTY(__name, __value) \
static PyObject *CandleFrameType_get_ ## __name ## _property(CandleFrameType_object *self, void *closure) { \
    return PyBool_FromLong(self->type & __value); \
} \
static int CandleFrameType_set_ ## __name ## _property(CandleFrameType_object *self, PyObject* value, void *closure) { \
    if (Py_IsTrue(value)) \
        self->type |= __value; \
    else \
        self->type &= ~__value; \
    return 0; \
}

CandleFrameType_PROPERTY(rx, CANDLE_FRAME_TYPE_RX)
CandleFrameType_PROPERTY(extended_id, CANDLE_FRAME_TYPE_EFF)
CandleFrameType_PROPERTY(remote_frame, CANDLE_FRAME_TYPE_RTR)
CandleFrameType_PROPERTY(error_frame, CANDLE_FRAME_TYPE_ERR)
CandleFrameType_PROPERTY(fd, CANDLE_FRAME_TYPE_FD)
CandleFrameType_PROPERTY(bitrate_switch, CANDLE_FRAME_TYPE_BRS)
CandleFrameType_PROPERTY(error_state_indicator, CANDLE_FRAME_TYPE_ESI)

#define CandleFrameType_GETSET(__name) \
{ \
    .name = #__name, \
    .get = (getter)CandleFrameType_get_ ## __name ## _property, \
    .set = (setter)CandleFrameType_set_ ## __name ## _property \
}

static PyGetSetDef CandleFrameType_getset[] = {
    CandleFrameType_GETSET(rx),
    CandleFrameType_GETSET(extended_id),
    CandleFrameType_GETSET(remote_frame),
    CandleFrameType_GETSET(error_frame),
    CandleFrameType_GETSET(fd),
    CandleFrameType_GETSET(bitrate_switch),
    CandleFrameType_GETSET(error_state_indicator),
    {NULL}
};

static PyTypeObject CandleFrameTypeType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "candle_api.CandleFrameType",
    .tp_basicsize = sizeof(CandleFrameType_object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc)CandleFrameType_init,
    .tp_getset = CandleFrameType_getset
};

/* CandleCanState */

/* CandleCanFrame */

typedef struct {
    PyObject_HEAD
    struct candle_can_frame frame;
} CandleCanFrame_object;

static PyTypeObject CandleCanFrameType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "candle_api.CandleCanFrame",
    .tp_basicsize = sizeof(CandleFrameType_object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew
};

/* CandleFeature */

typedef struct {
    PyObject_HEAD
    enum candle_feature feature;
} CandleFeature_object;

static PyObject *CandleFeature_get_listen_only_property(PyObject *self, void *closure) {
    CandleFeature_object *obj = (CandleFeature_object *)self;
    return PyBool_FromLong(obj->feature & CANDLE_FEATURE_LISTEN_ONLY);
}

static PyObject *CandleFeature_get_loop_back_property(PyObject *self, void *closure) {
    CandleFeature_object *obj = (CandleFeature_object *)self;
    return PyBool_FromLong(obj->feature & CANDLE_FEATURE_LOOP_BACK);
}

static PyObject *CandleFeature_get_triple_sample_property(PyObject *self, void *closure) {
    CandleFeature_object *obj = (CandleFeature_object *)self;
    return PyBool_FromLong(obj->feature & CANDLE_FEATURE_TRIPLE_SAMPLE);
}

static PyObject *CandleFeature_get_one_shot_property(PyObject *self, void *closure) {
    CandleFeature_object *obj = (CandleFeature_object *)self;
    return PyBool_FromLong(obj->feature & CANDLE_FEATURE_ONE_SHOT);
}

static PyObject *CandleFeature_get_hardware_timestamp_property(PyObject *self, void *closure) {
    CandleFeature_object *obj = (CandleFeature_object *)self;
    return PyBool_FromLong(obj->feature & CANDLE_FEATURE_HW_TIMESTAMP);
}

static PyObject *CandleFeature_get_pad_package_property(PyObject *self, void *closure) {
    CandleFeature_object *obj = (CandleFeature_object *)self;
    return PyBool_FromLong(obj->feature & CANDLE_FEATURE_PAD_PKTS_TO_MAX_PKT_SIZE);
}

static PyObject *CandleFeature_get_fd_property(PyObject *self, void *closure) {
    CandleFeature_object *obj = (CandleFeature_object *)self;
    return PyBool_FromLong(obj->feature & CANDLE_FEATURE_FD);
}

static PyObject *CandleFeature_get_bit_error_reporting_property(PyObject *self, void *closure) {
    CandleFeature_object *obj = (CandleFeature_object *)self;
    return PyBool_FromLong(obj->feature & CANDLE_FEATURE_BERR_REPORTING);
}

static PyObject *CandleFeature_get_termination_property(PyObject *self, void *closure) {
    CandleFeature_object *obj = (CandleFeature_object *)self;
    return PyBool_FromLong(obj->feature & CANDLE_FEATURE_TERMINATION);
}

static PyObject *CandleFeature_get_get_state_property(PyObject *self, void *closure) {
    CandleFeature_object *obj = (CandleFeature_object *)self;
    return PyBool_FromLong(obj->feature & CANDLE_FEATURE_GET_STATE);
}

static PyGetSetDef CandleFeature_getset[] = {
    {
        .name = "listen_only",
        .get = CandleFeature_get_listen_only_property
    },
    {
        .name = "loop_back",
        .get = CandleFeature_get_loop_back_property
    },
    {
        .name = "triple_sample",
        .get = CandleFeature_get_triple_sample_property
    },
    {
        .name = "one_shot",
        .get = CandleFeature_get_one_shot_property
    },
    {
        .name = "hardware_timestamp",
        .get = CandleFeature_get_hardware_timestamp_property
    },
    {
        .name = "pad_package",
        .get = CandleFeature_get_pad_package_property
    },
    {
        .name = "fd",
        .get = CandleFeature_get_fd_property
    },
    {
        .name = "bit_error_reporting",
        .get = CandleFeature_get_bit_error_reporting_property
    },
    {
        .name = "termination",
        .get = CandleFeature_get_termination_property
    },
    {
        .name = "get_state",
        .get = CandleFeature_get_get_state_property
    },
    {NULL}
};

static PyTypeObject CandleFeatureType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "candle_api.CandleFeature",
    .tp_basicsize = sizeof(CandleFeature_object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_getset = CandleFeature_getset
};

/* CandleBitTimingConst */

typedef struct {
    PyObject_HEAD
    struct candle_bit_timing_const bt_const;
} CandleBitTimingConst_object;

static PyMemberDef CandleBitTimingConst_members[] = {
    {
        .name = "tseg1_min",
        .type = Py_T_UINT,
        .offset = offsetof(CandleBitTimingConst_object, bt_const.tseg1_min),
        .flags = Py_READONLY
    },
    {
        .name = "tseg1_max",
        .type = Py_T_UINT,
        .offset = offsetof(CandleBitTimingConst_object, bt_const.tseg1_max),
        .flags = Py_READONLY
    },
    {
        .name = "tseg2_min",
        .type = Py_T_UINT,
        .offset = offsetof(CandleBitTimingConst_object, bt_const.tseg2_min),
        .flags = Py_READONLY
    },
    {
        .name = "tseg2_max",
        .type = Py_T_UINT,
        .offset = offsetof(CandleBitTimingConst_object, bt_const.tseg2_max),
        .flags = Py_READONLY
    },
    {
        .name = "sjw_max",
        .type = Py_T_UINT,
        .offset = offsetof(CandleBitTimingConst_object, bt_const.sjw_max),
        .flags = Py_READONLY
    },
    {
        .name = "brp_min",
        .type = Py_T_UINT,
        .offset = offsetof(CandleBitTimingConst_object, bt_const.brp_min),
        .flags = Py_READONLY
    },
    {
        .name = "brp_max",
        .type = Py_T_UINT,
        .offset = offsetof(CandleBitTimingConst_object, bt_const.brp_max),
        .flags = Py_READONLY
    },
    {
        .name = "brp_inc",
        .type = Py_T_UINT,
        .offset = offsetof(CandleBitTimingConst_object, bt_const.brp_inc),
        .flags = Py_READONLY
    },
    {NULL},
};

static PyTypeObject CandleBitTimingConstType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "candle_api.CandleBitTimingConst",
    .tp_basicsize = sizeof(CandleBitTimingConst_object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_members = CandleBitTimingConst_members
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

    CandleFeature_object *ft_obj = (CandleFeature_object*)PyObject_New(CandleFeature_object, &CandleFeatureType);
    if (ft_obj == NULL)
        return NULL;

    ft_obj->feature = obj->dev->channels[obj->ch].feature;

    return (PyObject*)ft_obj;
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

    if (PyType_Ready(&CandleFrameTypeType) < 0)
        return NULL;

    if (PyType_Ready(&CandleCanFrameType) < 0)
        return NULL;

    if (PyType_Ready(&CandleModeType) < 0)
        return NULL;

    if (PyType_Ready(&CandleFeatureType) < 0)
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

    Py_INCREF(&CandleFrameTypeType);
    if (PyModule_AddObject(m, "CandleFrameTypeType", (PyObject *)&CandleFrameTypeType) < 0) {
        Py_DECREF(&CandleFrameTypeType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&CandleCanFrameType);
    if (PyModule_AddObject(m, "CandleCanFrameType", (PyObject *)&CandleCanFrameType) < 0) {
        Py_DECREF(&CandleCanFrameType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&CandleModeType);
    if (PyModule_AddObject(m, "CandleModeType", (PyObject *)&CandleModeType) < 0) {
        Py_DECREF(&CandleModeType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&CandleFeatureType);
    if (PyModule_AddObject(m, "CandleFeatureType", (PyObject *)&CandleFeatureType) < 0) {
        Py_DECREF(&CandleFeatureType);
        Py_DECREF(m);
        return NULL;
    }

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
