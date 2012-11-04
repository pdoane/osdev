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

typedef struct UsbEndpoint
{
    UsbEndpDesc desc;
    uint toggle;
} UsbEndpoint;

// ------------------------------------------------------------------------------------------------
// USB Transfer

typedef struct UsbTransfer
{
    UsbEndpoint *endp;
    UsbDevReq *req;
    void *data;
    uint len;
    bool complete;
    bool success;
} UsbTransfer;

// ------------------------------------------------------------------------------------------------
// USB Device

typedef struct UsbDevice
{
    struct UsbDevice *parent;
    struct UsbDevice *next;
    void *hc;
    void *drv;

    uint port;
    uint speed;
    uint addr;
    uint maxPacketSize;

    UsbEndpoint endp;

    UsbIntfDesc intfDesc;

    void (*hcControl)(struct UsbDevice *dev, UsbTransfer *t);
    void (*hcIntr)(struct UsbDevice *dev, UsbTransfer *t);

    void (*drvPoll)(struct UsbDevice *dev);
} UsbDevice;

// ------------------------------------------------------------------------------------------------
// Globals

extern UsbDevice *g_usbDeviceList;

// ------------------------------------------------------------------------------------------------
// Functions

UsbDevice *UsbDevCreate();
bool UsbDevInit(UsbDevice *dev);
bool UsbDevRequest(UsbDevice *dev,
    uint type, uint request,
    uint value, uint index,
    uint len, void *data);
bool UsbDevGetLangs(UsbDevice *dev, u16 *langs);
bool UsbDevGetString(UsbDevice *dev, char *str, uint langId, uint strIndex);
bool UsbDevClearHalt(UsbDevice *dev);
