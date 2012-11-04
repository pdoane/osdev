// ------------------------------------------------------------------------------------------------
// usb/desc.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

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
// USB HUB Descriptor Types

#define USB_DESC_HUB                    0x29

// ------------------------------------------------------------------------------------------------
// USB Device Descriptor

typedef struct UsbDeviceDesc
{
    u8 len;
    u8 type;
    u16 usbVer;
    u8 devClass;
    u8 devSubClass;
    u8 devProtocol;
    u8 maxPacketSize;
    u16 vendorId;
    u16 productId;
    u16 deviceVer;
    u8 vendorStr;
    u8 productStr;
    u8 serialStr;
    u8 confCount;
} PACKED UsbDeviceDesc;

// ------------------------------------------------------------------------------------------------
// USB Configuration Descriptor

typedef struct UsbConfDesc
{
    u8 len;
    u8 type;
    u16 totalLen;
    u8 intfCount;
    u8 confValue;
    u8 confStr;
    u8 attributes;
    u8 maxPower;
} PACKED UsbConfDesc;

// ------------------------------------------------------------------------------------------------
// USB String Descriptor

typedef struct UsbStringDesc
{
    u8 len;
    u8 type;
    u16 str[];
} PACKED UsbStringDesc;

// ------------------------------------------------------------------------------------------------
// USB Interface Descriptor

typedef struct UsbIntfDesc
{
    u8 len;
    u8 type;
    u8 intfIndex;
    u8 altSetting;
    u8 endpCount;
    u8 intfClass;
    u8 intfSubClass;
    u8 intfProtocol;
    u8 intfStr;
} PACKED UsbIntfDesc;

// ------------------------------------------------------------------------------------------------
// USB Endpoint Descriptor

typedef struct UsbEndpDesc
{
    u8 len;
    u8 type;
    u8 addr;
    u8 attributes;
    u16 maxPacketSize;
    u8 interval;
} PACKED UsbEndpDesc;

// ------------------------------------------------------------------------------------------------
// USB HID Desciptor

typedef struct UsbHidDesc
{
    u8 len;
    u8 type;
    u16 hidVer;
    u8 countryCode;
    u8 descCount;
    u8 descType;
    u16 descLen;
} PACKED UsbHidDesc;

// ------------------------------------------------------------------------------------------------
// USB Hub Descriptor

typedef struct UsbHubDesc
{
    u8 len;
    u8 type;
    u8 portCount;
    u16 chars;
    u8 portPowerTime;
    u8 current;
    // removable/power control bits vary in size
} PACKED UsbHubDesc;

// Hub Characteristics
#define HUB_POWER_MASK                  0x03        // Logical Power Switching Mode
#define HUB_POWER_GLOBAL                0x00
#define HUB_POWER_INDIVIDUAL            0x01
#define HUB_COMPOUND                    0x04        // Part of a Compound Device
#define HUB_CURRENT_MASK                0x18        // Over-current Protection Mode
#define HUB_TT_TTI_MASK                 0x60        // TT Think Time
#define HUB_PORT_INDICATORS             0x80        // Port Indicators

// ------------------------------------------------------------------------------------------------
// Functions

void UsbPrintDeviceDesc(UsbDeviceDesc *desc);
void UsbPrintConfDesc(UsbConfDesc *desc);
void UsbPrintIntfDesc(UsbIntfDesc *desc);
void UsbPrintEndpDesc(UsbEndpDesc *desc);

void UsbPrintHidDesc(UsbHidDesc *desc);
void UsbPrintHubDesc(UsbHubDesc *desc);
