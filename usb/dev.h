// ------------------------------------------------------------------------------------------------
// usb_dev.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "usb/desc.h"
#include "usb/req.h"

// ------------------------------------------------------------------------------------------------
// USB Limits

#define USB_STRING_SIZE                 127

// ------------------------------------------------------------------------------------------------
// USB Speeds

#define USB_FULL_SPEED                  0x00
#define USB_LOW_SPEED                   0x01
#define USB_HIGH_SPEED                  0x02

// ------------------------------------------------------------------------------------------------
// USB Endpoint

typedef struct USB_Endpoint
{
    USB_EndpDesc desc;
    uint toggle;
} USB_Endpoint;

// ------------------------------------------------------------------------------------------------
// USB Transfer

typedef struct USB_Transfer
{
    USB_Endpoint* endp;
    USB_DevReq* req;
    void* data;
    uint len;
    bool complete;
    bool success;
} USB_Transfer;

// ------------------------------------------------------------------------------------------------
// USB Device

typedef struct USB_Device
{
    struct USB_Device* parent;
    struct USB_Device* next;
    void* hc;
    void* drv;

    uint port;
    uint speed;
    uint addr;
    uint max_packet_size;

    USB_Endpoint endp;

    USB_IntfDesc intf_desc;

    void (*hc_control)(struct USB_Device* dev, USB_Transfer* t);
    void (*hc_intr)(struct USB_Device* dev, USB_Transfer* t);

    void (*drv_poll)(struct USB_Device* dev);
} USB_Device;

// ------------------------------------------------------------------------------------------------
// Globals

extern USB_Device* usb_dev_list;

// ------------------------------------------------------------------------------------------------
// Functions

USB_Device* usb_dev_create();
bool usb_dev_init(USB_Device* dev);
bool usb_dev_request(USB_Device* dev,
    uint type, uint request,
    uint value, uint index,
    uint len, void* data);
bool usb_dev_get_langs(USB_Device* dev, u16* langs);
bool usb_dev_get_string(USB_Device* dev, char* str, uint lang_id, uint str_index);
bool usb_dev_clear_halt(USB_Device* dev);
