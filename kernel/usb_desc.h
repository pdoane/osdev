// ------------------------------------------------------------------------------------------------
// usb_desc.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"

// ------------------------------------------------------------------------------------------------
// USB Base Descriptor Types

#define USB_DESC_DEVICE                 0x01
#define USB_DESC_CONF                   0x02
#define USB_DESC_STRING                 0x03
#define USB_DESC_INTF                   0x04
#define USB_DESC_ENDP                   0x05

// ------------------------------------------------------------------------------------------------
// USB HID Descriptor Types

#define USB_DESC_HID                    0x21
#define USB_DESC_REPORT                 0x22
#define USB_DESC_PHYSICAL               0x23

// ------------------------------------------------------------------------------------------------
// USB Device Descriptor

typedef struct USB_DeviceDesc
{
    u8 len;
    u8 type;
    u16 usb_ver;
    u8 dev_class;
    u8 dev_subclass;
    u8 dev_protocol;
    u8 max_packet_size;
    u16 vendor_id;
    u16 product_id;
    u16 device_ver;
    u8 vendor_str;
    u8 product_str;
    u8 serial_str;
    u8 conf_count;
} PACKED USB_DeviceDesc;

// ------------------------------------------------------------------------------------------------
// USB Configuration Descriptor

typedef struct USB_ConfDesc
{
    u8 len;
    u8 type;
    u16 total_len;
    u8 intf_count;
    u8 conf_value;
    u8 conf_str;
    u8 attributes;
    u8 max_power;
} PACKED USB_ConfDesc;

// ------------------------------------------------------------------------------------------------
// USB String Descriptor

typedef struct USB_StringDesc
{
    u8 len;
    u8 type;
    u16 str[];
} PACKED USB_StringDesc;

// ------------------------------------------------------------------------------------------------
// USB Interface Descriptor

typedef struct USB_IntfDesc
{
    u8 len;
    u8 type;
    u8 intf_index;
    u8 alt_setting;
    u8 endp_count;
    u8 intf_class;
    u8 intf_subclass;
    u8 intf_protocol;
    u8 intf_str;
} PACKED USB_IntfDesc;

// ------------------------------------------------------------------------------------------------
// USB Endpoint Descriptor

typedef struct USB_EndpDesc
{
    u8 len;
    u8 type;
    u8 addr;
    u8 attributes;
    u16 max_packet_size;
    u8 interval;
} PACKED USB_EndpDesc;

// ------------------------------------------------------------------------------------------------
// USB HID Desciptor

typedef struct USB_HidDesc
{
    u8 len;
    u8 type;
    u16 hid_ver;
    u8 country_code;
    u8 desc_count;
    u8 desc_type;
    u16 desc_len;
} PACKED USB_HidDesc;

// ------------------------------------------------------------------------------------------------
// Functions

void usb_print_device_desc(USB_DeviceDesc* desc);
void usb_print_conf_desc(USB_ConfDesc* desc);
void usb_print_intf_desc(USB_IntfDesc* desc);
void usb_print_endp_desc(USB_EndpDesc* desc);

void usb_print_hid_desc(USB_HidDesc* hid_desc);
