// ------------------------------------------------------------------------------------------------
// usb_req.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"

// ------------------------------------------------------------------------------------------------
// USB Request Type

#define RT_TRANSFER_MASK                0x80
#define RT_DEV_TO_HOST                  0x80
#define RT_HOST_TO_DEV                  0x00

#define RT_TYPE_MASK                    0x60
#define RT_STANDARD                     0x00
#define RT_CLASS                        0x20
#define RT_VENDOR                       0x40

#define RT_RECIPIENT_MASK               0x1f
#define RT_DEV                          0x00
#define RT_INTF                         0x01
#define RT_ENDP                         0x02
#define RT_OTHER                        0x03

// ------------------------------------------------------------------------------------------------
// USB Device Requests

#define REQ_GET_STATUS                  0x00
#define REQ_CLEAR_FEATURE               0x01
#define REQ_SET_FEATURE                 0x03
#define REQ_SET_ADDR                    0x05
#define REQ_GET_DESC                    0x06
#define REQ_SET_DESC                    0x07
#define REQ_GET_CONF                    0x08
#define REQ_SET_CONF                    0x09
#define REQ_GET_INTF                    0x0a
#define REQ_SET_INTF                    0x0b
#define REQ_SYNC_FRAME                  0x0c

// ------------------------------------------------------------------------------------------------
// USB Hub Class Requests

#define REQ_CLEAR_TT_BUFFER             0x08
#define REQ_RESET_TT                    0x09
#define REQ_GET_TT_STATE                0x0a
#define REQ_STOP_TT                     0x0b

// ------------------------------------------------------------------------------------------------
// USB HID Interface Requests

#define REQ_GET_REPORT                  0x01
#define REQ_GET_IDLE                    0x02
#define REQ_GET_PROTOCOL                0x03
#define REQ_SET_REPORT                  0x09
#define REQ_SET_IDLE                    0x0a
#define REQ_SET_PROTOCOL                0x0b

// ------------------------------------------------------------------------------------------------
// USB Standard Feature Selectors

#define F_DEVICE_REMOTE_WAKEUP          1   // Device
#define F_ENDPOINT_HALT                 2   // Endpoint
#define F_TEST_MODE                     3   // Device

// ------------------------------------------------------------------------------------------------
// USB Hub Feature Seletcors

#define F_C_HUB_LOCAL_POWER             0   // Hub
#define F_C_HUB_OVER_CURRENT            1   // Hub
#define F_PORT_CONNECTION               0   // Port
#define F_PORT_ENABLE                   1   // Port
#define F_PORT_SUSPEND                  2   // Port
#define F_PORT_OVER_CURRENT             3   // Port
#define F_PORT_RESET                    4   // Port
#define F_PORT_POWER                    8   // Port
#define F_PORT_LOW_SPEED                9   // Port
#define F_C_PORT_CONNECTION             16  // Port
#define F_C_PORT_ENABLE                 17  // Port
#define F_C_PORT_SUSPEND                18  // Port
#define F_C_PORT_OVER_CURRENT           19  // Port
#define F_C_PORT_RESET                  20  // Port
#define F_PORT_TEST                     21  // Port
#define F_PORT_INDICATOR                22  // Port

// ------------------------------------------------------------------------------------------------
// USB Device Request

typedef struct USB_DevReq
{
    u8 type;
    u8 req;
    u16 value;
    u16 index;
    u16 len;
} PACKED USB_DevReq;
