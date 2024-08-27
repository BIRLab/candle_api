#ifndef CANDLE_API_H
#define CANDLE_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

enum candle_feature {
    CANDLE_FEATURE_LISTEN_ONLY = 1 << 0,
    CANDLE_FEATURE_LOOP_BACK = 1 << 1,
    CANDLE_FEATURE_TRIPLE_SAMPLE = 1 << 2,
    CANDLE_FEATURE_ONE_SHOT = 1 << 3,
    CANDLE_FEATURE_HW_TIMESTAMP = 1 << 4,
    CANDLE_FEATURE_IDENTIFY = 1 << 5,
    CANDLE_FEATURE_USER_ID = 1 << 6,
    CANDLE_FEATURE_PAD_PKTS_TO_MAX_PKT_SIZE = 1 << 7,
    CANDLE_FEATURE_FD = 1 << 8,
    CANDLE_FEATURE_REQ_USB_QUIRK_LPC546XX = 1 << 9,
    CANDLE_FEATURE_BT_CONST_EXT = 1 << 10,
    CANDLE_FEATURE_TERMINATION = 1 << 11,
    CANDLE_FEATURE_BERR_REPORTING = 1 << 12,
    CANDLE_FEATURE_GET_STATE = 1 << 13,
    CANDLE_FEATURE_QUIRK_BREQ_CANTACT_PRO = 1 << 14
};

enum candle_mode {
    CANDLE_MODE_NORMAL = 0,
    CANDLE_MODE_LISTEN_ONLY = 1 << 0,
    CANDLE_MODE_LOOP_BACK = 1 << 1,
    CANDLE_MODE_TRIPLE_SAMPLE = 1 << 2,
    CANDLE_MODE_ONE_SHOT = 1 << 3,
    CANDLE_MODE_HW_TIMESTAMP = 1 << 4,
    CANDLE_MODE_PAD_PKTS_TO_MAX_PKT_SIZE = 1 << 7,
    CANDLE_MODE_FD = 1 << 8,
    CANDLE_MODE_BERR_REPORTING = 1 << 12
};

enum candle_frame_type {
    CANDLE_FRAME_TYPE_RX = 1 << 0,
    CANDLE_FRAME_TYPE_EFF = 1 << 1,
    CANDLE_FRAME_TYPE_RTR = 1 << 2,
    CANDLE_FRAME_TYPE_ERR = 1 << 3,
    CANDLE_FRAME_TYPE_FD = 1 << 4,
    CANDLE_FRAME_TYPE_BRS = 1 << 5,
    CANDLE_FRAME_TYPE_ESI = 1 << 6
};

enum candle_can_state {
    CANDLE_CAN_STATE_ERROR_ACTIVE = 0,
    CANDLE_CAN_STATE_ERROR_WARNING,
    CANDLE_CAN_STATE_ERROR_PASSIVE,
    CANDLE_CAN_STATE_BUS_OFF,
    CANDLE_CAN_STATE_STOPPED,
    CANDLE_CAN_STATE_SLEEPING
};

struct candle_state {
    enum candle_can_state state;
    uint32_t rxerr;
    uint32_t txerr;
};

struct candle_bit_timing_const {
    uint32_t tseg1_min;
    uint32_t tseg1_max;
    uint32_t tseg2_min;
    uint32_t tseg2_max;
    uint32_t sjw_max;
    uint32_t brp_min;
    uint32_t brp_max;
    uint32_t brp_inc;
};

struct candle_bit_timing {
    uint32_t prop_seg;
    uint32_t phase_seg1;
    uint32_t phase_seg2;
    uint32_t sjw;
    uint32_t brp;
};

struct candle_can_frame {
    enum candle_frame_type type;
    uint32_t can_id;
    uint8_t can_dlc;
    uint8_t data[64];
    uint32_t timestamp_us;
};

struct candle_channel {
    enum candle_feature feature;                    // read only
    uint32_t clock_frequency;                       // read only
    struct {
        struct candle_bit_timing_const nominal;     // read only
        struct candle_bit_timing_const data;        // read only
    } bit_timing_const;
};

struct candle_device {
    struct candle_device_handle *handle;    // reserve
    bool is_connected;                      // read only
    bool is_open;                           // read only
    uint16_t vendor_id;                     // read only
    uint16_t product_id;                    // read only
    char manufacturer[256];                 // read only
    char product[256];                      // read only
    char serial_number[256];                // read only
    uint8_t channel_count;                  // read only
    uint32_t software_version;              // read only
    uint32_t hardware_version;              // read only
    struct candle_channel channels[];       // read only (size == channel_count)
};

bool candle_initialize(void);
void candle_finalize(void);
bool candle_get_device_list(struct candle_device ***devices, size_t *size);
void candle_free_device_list(struct candle_device **devices);
struct candle_device *candle_ref_device(struct candle_device *device);
void candle_unref_device(struct candle_device *device);
bool candle_open_device(struct candle_device *device);
void candle_close_device(struct candle_device *device);
bool candle_reset_channel(struct candle_device *device, uint8_t channel);
bool candle_start_channel(struct candle_device *device, uint8_t channel, enum candle_mode mode);
bool candle_set_bit_timing(struct candle_device *device, uint8_t channel, struct candle_bit_timing *bit_timing);
bool candle_set_data_bit_timing(struct candle_device *device, uint8_t channel, struct candle_bit_timing *bit_timing);
bool candle_get_termination(struct candle_device *device, uint8_t channel, bool *enable);
bool candle_set_termination(struct candle_device *device, uint8_t channel, bool enable);
bool candle_get_state(struct candle_device *device, uint8_t channel, struct candle_state *state);
bool candle_send_frame_nowait(struct candle_device *device, uint8_t channel, struct candle_can_frame *frame);
bool candle_send_frame(struct candle_device *device, uint8_t channel, struct candle_can_frame *frame, uint32_t milliseconds);
bool candle_receive_frame_nowait(struct candle_device *device, uint8_t channel, struct candle_can_frame *frame);
bool candle_receive_frame(struct candle_device *device, uint8_t channel, struct candle_can_frame *frame, uint32_t milliseconds);

#ifdef __cplusplus
}
#endif

#endif // CANDLE_API_H
