#define PY_SSIZE_T_CLEAN
#include <Python.h>

#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION < 12
#include "structmember.h"
#define Py_T_UINT T_UINT
#define Py_READONLY READONLY
#endif

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

typedef struct {
    PyObject_HEAD
    enum candle_can_state can_state;
} CandleCanState_object;

static PyObject* CandleCanState_richcompare(PyObject* self, PyObject* other, int op) {
    if (op != Py_EQ) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (!PyObject_TypeCheck(other, Py_TYPE(self))) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    CandleCanState_object* self_obj = (CandleCanState_object*)self;
    CandleCanState_object* other_obj = (CandleCanState_object*)other;

    if (self_obj->can_state == other_obj->can_state) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static PyTypeObject CandleCanStateType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "candle_api.CandleCanState",
    .tp_basicsize = sizeof(CandleCanState_object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_richcompare = CandleCanState_richcompare
};

/* CandleState */

typedef struct {
    PyObject_HEAD
    struct candle_state state;
} CandleState_object;

static PyObject *CandleState_get_state_property(CandleState_object *self, void *closure) {
    CandleCanState_object *obj = (CandleCanState_object*)PyObject_New(CandleCanState_object, &CandleCanStateType);
    if (obj == NULL)
        return NULL;

    obj->can_state = self->state.state;

    return (PyObject*)obj;
}

static PyObject *CandleState_get_rx_error_count_property(CandleState_object *self, void *closure) {
    return PyLong_FromLong((long)self->state.rxerr);
}

static PyObject *CandleState_get_tx_error_count_property(CandleState_object *self, void *closure) {
    return PyLong_FromLong((long)self->state.txerr);
}

static PyGetSetDef CandleState_getset[] = {
    {
        .name = "state",
        .get = (getter)CandleState_get_state_property
    },
    {
        .name = "rx_error_count",
        .get = (getter)CandleState_get_rx_error_count_property
    },
    {
        .name = "tx_error_count",
        .get = (getter)CandleState_get_tx_error_count_property
    },
    {NULL}
};

static PyTypeObject CandleStateType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "candle_api.CandleState",
    .tp_basicsize = sizeof(CandleCanState_object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_getset = CandleState_getset
};

/* CandleCanFrame */

static const uint8_t dlc2len[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};

typedef struct {
    PyObject_HEAD
    struct candle_can_frame frame;
    Py_ssize_t size;
} CandleCanFrame_object;

static int CandleCanFrame_init(CandleCanFrame_object *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {
        "frame_type",
        "can_id",
        "can_dlc",
        "data",
        NULL
    };

    CandleFrameType_object *frame_type;
    unsigned int can_id;
    unsigned int can_dlc;
    Py_buffer data;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OIIy*", kwlist, &frame_type, &can_id, &can_dlc, &data))
        return -1;

    if (can_dlc > 15) {
        PyBuffer_Release(&data);
        return -1;
    }

    self->size = dlc2len[can_dlc];
    self->frame.type = frame_type->type;
    self->frame.can_id = can_id;
    self->frame.can_dlc = can_dlc;
    memset(self->frame.data, 0, sizeof(self->frame.data));
    memcpy(self->frame.data, data.buf, data.len < self->size ? data.len : self->size);

    PyBuffer_Release(&data);
    return 0;
}

static int CandleCanFrame_getbuffer(CandleCanFrame_object *self, Py_buffer *view, int flags) {
    if (view == NULL) {
        PyErr_SetString(PyExc_ValueError, "NULL view in getbuffer");
        return -1;
    }

    view->buf = self->frame.data;
    view->obj = (PyObject *)self;
    view->len = self->size;
    view->readonly = 0;
    view->itemsize = 1;
    view->ndim = 1;
    view->format = NULL;
    view->shape = &self->size;
    view->strides = &view->itemsize;
    view->suboffsets = NULL;
    view->internal = NULL;

    Py_INCREF(self);

    return 0;
}

static PyBufferProcs CandleCanFrame_as_buffer = {
    .bf_getbuffer = (getbufferproc)CandleCanFrame_getbuffer,
    .bf_releasebuffer = NULL,
};

static PyObject *CandleCanFrame_get_frame_type_property(CandleCanFrame_object *self, void *closure) {
    CandleFrameType_object *ft_obj = (CandleFrameType_object*)PyObject_New(CandleFrameType_object, &CandleFrameTypeType);
    if (ft_obj == NULL)
        return NULL;

    ft_obj->type = self->frame.type;

    return (PyObject*)ft_obj;
}

static int CandleCanFrame_set_frame_type_property(CandleCanFrame_object *self, PyObject* value, void *closure) { \
    if (!PyObject_IsInstance(value, (PyObject*)&CandleFrameTypeType))
        return -1;

    self->frame.type = ((CandleFrameType_object *)value)->type;

    return 0;
}

static PyObject *CandleCanFrame_get_can_id_property(CandleCanFrame_object *self, void *closure) {
    return PyLong_FromLong((long)self->frame.can_id);
}

static int CandleCanFrame_set_can_id_property(CandleCanFrame_object *self, PyObject* value, void *closure) { \
    if (!PyLong_Check(value))
        return -1;

    long can_id = PyLong_AsLong(value);
    self->frame.can_id = can_id;

    return 0;
}

static PyObject *CandleCanFrame_get_can_dlc_property(CandleCanFrame_object *self, void *closure) {
    return PyLong_FromLong((long)self->frame.can_dlc);
}

static int CandleCanFrame_set_can_dlc_property(CandleCanFrame_object *self, PyObject* value, void *closure) { \
    if (!PyLong_Check(value))
        return -1;

    long can_dlc = PyLong_AsLong(value);

    if (can_dlc < 0 || can_dlc > 15)
        return -1;

    self->frame.can_dlc = can_dlc;
    self->size = dlc2len[can_dlc];

    return 0;
}

static PyObject *CandleCanFrame_get_timestamp_property(CandleCanFrame_object *self, void *closure) {
    return PyFloat_FromDouble((double)self->frame.timestamp_us / 1e6);
}

static PyGetSetDef CandleCanFrame_getset[] = {
    {
        .name = "frame_type",
        .get = (getter)CandleCanFrame_get_frame_type_property,
        .set = (setter)CandleCanFrame_set_frame_type_property
    },
    {
        .name = "can_id",
        .get = (getter)CandleCanFrame_get_can_id_property,
        .set = (setter)CandleCanFrame_set_can_id_property
    },
    {
        .name = "can_dlc",
        .get = (getter)CandleCanFrame_get_can_dlc_property,
        .set = (setter)CandleCanFrame_set_can_dlc_property
    },
    {
        .name = "timestamp",
        .get = (getter)CandleCanFrame_get_timestamp_property
    },
    {NULL}
};

static PyTypeObject CandleCanFrameType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "candle_api.CandleCanFrame",
    .tp_basicsize = sizeof(CandleCanFrame_object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc)CandleCanFrame_init,
    .tp_as_buffer = &CandleCanFrame_as_buffer,
    .tp_getset = CandleCanFrame_getset
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
    if (candle_open_device(self->dev)) {
        candle_unref_device(self->dev);
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *CandleDevice_close(CandleDevice_object *self, PyObject *Py_UNUSED(ignored))
{
    if (self->dev->is_open) {
        candle_ref_device(self->dev);
        candle_close_device(self->dev);
    }
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
        {"list_device", list_device, METH_NOARGS, NULL},
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

    if (PyType_Ready(&CandleCanStateType) < 0)
        return NULL;

    if (PyType_Ready(&CandleStateType) < 0)
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
    if (PyModule_AddObject(m, "CandleFrameType", (PyObject *)&CandleFrameTypeType) < 0) {
        Py_DECREF(&CandleFrameTypeType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&CandleCanStateType);
    if (PyModule_AddObject(m, "CandleCanState", (PyObject *)&CandleCanStateType) < 0) {
        Py_DECREF(&CandleCanStateType);
        Py_DECREF(m);
        return NULL;
    }

#define ADD_CAN_STATE_ENUM(__name) \
    can_state_enum = (CandleCanState_object*)PyObject_New(CandleCanState_object, &CandleCanStateType); \
    if (can_state_enum == NULL) \
        return NULL; \
    can_state_enum->can_state = CANDLE_CAN_STATE_ ## __name; \
    if (PyDict_SetItemString(CandleCanStateType.tp_dict, #__name, (PyObject *)can_state_enum) < 0) { \
        Py_DECREF(can_state_enum); \
        return NULL; \
    }

    CandleCanState_object *can_state_enum;
    ADD_CAN_STATE_ENUM(ERROR_ACTIVE);
    ADD_CAN_STATE_ENUM(ERROR_WARNING);
    ADD_CAN_STATE_ENUM(ERROR_PASSIVE);
    ADD_CAN_STATE_ENUM(BUS_OFF);
    ADD_CAN_STATE_ENUM(STOPPED);
    ADD_CAN_STATE_ENUM(SLEEPING);

    Py_INCREF(&CandleStateType);
    if (PyModule_AddObject(m, "CandleState", (PyObject *)&CandleStateType) < 0) {
        Py_DECREF(&CandleStateType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&CandleCanFrameType);
    if (PyModule_AddObject(m, "CandleCanFrame", (PyObject *)&CandleCanFrameType) < 0) {
        Py_DECREF(&CandleCanFrameType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&CandleModeType);
    if (PyModule_AddObject(m, "CandleMode", (PyObject *)&CandleModeType) < 0) {
        Py_DECREF(&CandleModeType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&CandleFeatureType);
    if (PyModule_AddObject(m, "CandleFeature", (PyObject *)&CandleFeatureType) < 0) {
        Py_DECREF(&CandleFeatureType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&CandleBitTimingConstType);
    if (PyModule_AddObject(m, "CandleBitTimingConst", (PyObject *)&CandleBitTimingConstType) < 0) {
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
