#ifndef GS_USB_DEF_H
#define GS_USB_DEF_H

#include <stdint.h>

#pragma pack(push)
#pragma pack(1)
struct gs_host_config {
    uint32_t byte_order;
};

struct gs_device_config {
    uint8_t reserved1;
    uint8_t reserved2;
    uint8_t reserved3;
    uint8_t icount;
    uint32_t sw_version;
    uint32_t hw_version;
};

struct gs_device_bt_const {
    uint32_t feature;
    uint32_t fclk_can;
    uint32_t tseg1_min;
    uint32_t tseg1_max;
    uint32_t tseg2_min;
    uint32_t tseg2_max;
    uint32_t sjw_max;
    uint32_t brp_min;
    uint32_t brp_max;
    uint32_t brp_inc;
};

struct gs_device_bt_const_extended {
    uint32_t feature;
    uint32_t fclk_can;
    uint32_t tseg1_min;
    uint32_t tseg1_max;
    uint32_t tseg2_min;
    uint32_t tseg2_max;
    uint32_t sjw_max;
    uint32_t brp_min;
    uint32_t brp_max;
    uint32_t brp_inc;

    uint32_t dtseg1_min;
    uint32_t dtseg1_max;
    uint32_t dtseg2_min;
    uint32_t dtseg2_max;
    uint32_t dsjw_max;
    uint32_t dbrp_min;
    uint32_t dbrp_max;
    uint32_t dbrp_inc;
};

struct gs_device_mode {
    uint32_t mode;
    uint32_t flags;
};

struct gs_device_bittiming {
    uint32_t prop_seg;
    uint32_t phase_seg1;
    uint32_t phase_seg2;
    uint32_t sjw;
    uint32_t brp;
};

struct gs_device_state {
    uint32_t state;
    uint32_t rxerr;
    uint32_t txerr;
};

struct classic_can {
    uint8_t data[8];
};

struct classic_can_ts {
    uint8_t data[8];
    uint32_t timestamp_us;
};

struct classic_can_quirk {
    uint8_t data[8];
    uint8_t quirk;
};

struct canfd {
    uint8_t data[64];
};

struct canfd_ts {
    uint8_t data[64];
    uint32_t timestamp_us;
};

struct canfd_quirk {
    uint8_t data[64];
    uint8_t quirk;
};


struct gs_host_frame {
    uint32_t echo_id;
    uint32_t can_id;

    uint8_t can_dlc;
    uint8_t channel;
    uint8_t flags;
    uint8_t reserved;

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
    union {
        DECLARE_FLEX_ARRAY(struct classic_can, classic_can);
        DECLARE_FLEX_ARRAY(struct classic_can_ts, classic_can_ts);
        DECLARE_FLEX_ARRAY(struct classic_can_quirk, classic_can_quirk);
        DECLARE_FLEX_ARRAY(struct canfd, canfd);
        DECLARE_FLEX_ARRAY(struct canfd_ts, canfd_ts);
        DECLARE_FLEX_ARRAY(struct canfd_quirk, canfd_quirk);
    };
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
};
#pragma pack(pop)

enum gs_usb_breq {
    GS_USB_BREQ_HOST_FORMAT = 0,
    GS_USB_BREQ_BITTIMING,
    GS_USB_BREQ_MODE,
    GS_USB_BREQ_BERR,
    GS_USB_BREQ_BT_CONST,
    GS_USB_BREQ_DEVICE_CONFIG,
    GS_USB_BREQ_TIMESTAMP,
    GS_USB_BREQ_IDENTIFY,
    GS_USB_BREQ_GET_USER_ID,
    GS_USB_BREQ_QUIRK_CANTACT_PRO_DATA_BITTIMING = GS_USB_BREQ_GET_USER_ID,
    GS_USB_BREQ_SET_USER_ID,
    GS_USB_BREQ_DATA_BITTIMING,
    GS_USB_BREQ_BT_CONST_EXT,
    GS_USB_BREQ_SET_TERMINATION,
    GS_USB_BREQ_GET_TERMINATION,
    GS_USB_BREQ_GET_STATE,
};

#define CAN_EFF_FLAG    0x80000000U /* EFF/SFF is set in the MSB */
#define CAN_RTR_FLAG    0x40000000U /* remote transmission request */
#define CAN_ERR_FLAG    0x20000000U /* error message frame */

#define GS_CAN_FLAG_OVERFLOW    (1<<0)
#define GS_CAN_FLAG_FD          (1<<1) /* is a CAN-FD frame */
#define GS_CAN_FLAG_BRS         (1<<2) /* bit rate switch (for CAN-FD frames) */
#define GS_CAN_FLAG_ESI         (1<<3) /* error state indicator (for CAN-FD frames) */

#endif // GS_USB_DEF_H
