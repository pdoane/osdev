// ------------------------------------------------------------------------------------------------
// usb/dev.c
// ------------------------------------------------------------------------------------------------

#include "usb/dev.h"
#include "usb/driver.h"
#include "console/console.h"
#include "mem/vm.h"
#include "time/pit.h"

// ------------------------------------------------------------------------------------------------
UsbDevice *g_usbDeviceList;

static int s_nextUsbAddr;

// ------------------------------------------------------------------------------------------------
UsbDevice *UsbDevCreate()
{
    // Initialize structure
    UsbDevice *dev = VMAlloc(sizeof(UsbDevice));
    if (dev)
    {
        dev->parent = 0;
        dev->next = g_usbDeviceList;
        dev->hc = 0;
        dev->drv = 0;

        dev->port = 0;
        dev->speed = 0;
        dev->addr = 0;
        dev->maxPacketSize = 0;
        dev->endp.toggle = 0;

        dev->hcControl = 0;
        dev->hcIntr = 0;
        dev->drvPoll = 0;

        g_usbDeviceList = dev;
    }

    return dev;
}

// ------------------------------------------------------------------------------------------------
bool UsbDevInit(UsbDevice *dev)
{
    // Get first 8 bytes of device descriptor
    UsbDeviceDesc devDesc;
    if (!UsbDevRequest(dev,
        RT_DEV_TO_HOST | RT_STANDARD | RT_DEV,
        REQ_GET_DESC, (USB_DESC_DEVICE << 8) | 0, 0,
        8, &devDesc))
    {
        return false;
    }

    dev->maxPacketSize = devDesc.maxPacketSize;

    // Set address
    uint addr = ++s_nextUsbAddr;

    if (!UsbDevRequest(dev,
        RT_HOST_TO_DEV | RT_STANDARD | RT_DEV,
        REQ_SET_ADDR, addr, 0,
        0, 0))
    {
        return false;
    }

    dev->addr = addr;

    PitWait(2);    // Set address recovery time

    // Read entire descriptor
    if (!UsbDevRequest(dev,
        RT_DEV_TO_HOST | RT_STANDARD | RT_DEV,
        REQ_GET_DESC, (USB_DESC_DEVICE << 8) | 0, 0,
        sizeof(UsbDeviceDesc), &devDesc))
    {
        return false;
    }

    // Dump descriptor
    UsbPrintDeviceDesc(&devDesc);

    // String Info
    u16 langs[USB_STRING_SIZE];
    UsbDevGetLangs(dev, langs);

    uint langId = langs[0];
    if (langId)
    {
        char productStr[USB_STRING_SIZE];
        char vendorStr[USB_STRING_SIZE];
        char serialStr[USB_STRING_SIZE];
        UsbDevGetString(dev, productStr, langId, devDesc.productStr);
        UsbDevGetString(dev, vendorStr, langId, devDesc.vendorStr);
        UsbDevGetString(dev, serialStr, langId, devDesc.serialStr);
        ConsolePrint("  Product='%s' Vendor='%s' Serial=%s\n", productStr, vendorStr, serialStr);
    }

    // Pick configuration and interface - grab first for now
    u8 configBuf[256];
    uint pickedConfValue = 0;
    UsbIntfDesc *pickedIntfDesc = 0;
    UsbEndpDesc *pickedEndpDesc = 0;

    for (uint confIndex = 0; confIndex < devDesc.confCount; ++confIndex)
    {
        // Get configuration total length
        if (!UsbDevRequest(dev,
            RT_DEV_TO_HOST | RT_STANDARD | RT_DEV,
            REQ_GET_DESC, (USB_DESC_CONF << 8) | confIndex, 0,
            4, configBuf))
        {
            continue;
        }

        // Only static size supported for now
        UsbConfDesc *confDesc = (UsbConfDesc *)configBuf;
        if (confDesc->totalLen > sizeof(configBuf))
        {
            ConsolePrint("  Configuration length %d greater than %d bytes",
                confDesc->totalLen, sizeof(configBuf));
            continue;
        }

        // Read all configuration data
        if (!UsbDevRequest(dev,
            RT_DEV_TO_HOST | RT_STANDARD | RT_DEV,
            REQ_GET_DESC, (USB_DESC_CONF << 8) | confIndex, 0,
            confDesc->totalLen, configBuf))
        {
            continue;
        }

        UsbPrintConfDesc(confDesc);

        if (!pickedConfValue)
        {
            pickedConfValue = confDesc->confValue;
        }

        // Parse configuration data
        u8 *data = configBuf + confDesc->len;
        u8 *end = configBuf + confDesc->totalLen;

        while (data < end)
        {
            u8 len = data[0];
            u8 type = data[1];

            switch (type)
            {
            case USB_DESC_INTF:
                {
                    UsbIntfDesc *intfDesc = (UsbIntfDesc *)data;
                    UsbPrintIntfDesc(intfDesc);

                    if (!pickedIntfDesc)
                    {
                        pickedIntfDesc = intfDesc;
                    }
                }
                break;

            case USB_DESC_ENDP:
                {
                    UsbEndpDesc *endp_desc = (UsbEndpDesc *)data;
                    UsbPrintEndpDesc(endp_desc);

                    if (!pickedEndpDesc)
                    {
                        pickedEndpDesc = endp_desc;
                    }
                }
                break;
            }

            data += len;
        }
    }

    // Configure device
    if (pickedConfValue && pickedIntfDesc && pickedEndpDesc)
    {
        if (!UsbDevRequest(dev,
            RT_HOST_TO_DEV | RT_STANDARD | RT_DEV,
            REQ_SET_CONF, pickedConfValue, 0,
            0, 0))
        {
            return false;
        }

        dev->intfDesc = *pickedIntfDesc;
        dev->endp.desc = *pickedEndpDesc;

        // Initialize driver
        const UsbDriver *driver = g_usbDriverTable;
        while (driver->init)
        {
            if (driver->init(dev))
            {
                break;
            }

            ++driver;
        }
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
bool UsbDevRequest(UsbDevice *dev,
    uint type, uint request,
    uint value, uint index,
    uint len, void *data)
{
    UsbDevReq req;
    req.type = type;
    req.req = request;
    req.value = value;
    req.index = index;
    req.len = len;

    UsbTransfer t;
    t.endp = 0;
    t.req = &req;
    t.data = data;
    t.len = len;
    t.complete = false;
    t.success = false;

    dev->hcControl(dev, &t);

    return t.success;
}

// ------------------------------------------------------------------------------------------------
bool UsbDevGetLangs(UsbDevice *dev, u16 *langs)
{
    langs[0] = 0;

    u8 buf[256];
    UsbStringDesc *desc = (UsbStringDesc *)buf;

    // Get length
    if (!UsbDevRequest(dev,
        RT_DEV_TO_HOST | RT_STANDARD | RT_DEV,
        REQ_GET_DESC, (USB_DESC_STRING << 8) | 0, 0,
        1, desc))
    {
        return false;
    }

    // Get lang data
    if (!UsbDevRequest(dev,
        RT_DEV_TO_HOST | RT_STANDARD | RT_DEV,
        REQ_GET_DESC, (USB_DESC_STRING << 8) | 0, 0,
        desc->len, desc))
    {
        return false;
    }

    uint langLen = (desc->len - 2) / 2;
    for (uint i = 0; i < langLen; ++i)
    {
        langs[i] = desc->str[i];
    }

    langs[langLen] = 0;
    return true;
}

// ------------------------------------------------------------------------------------------------
bool UsbDevGetString(UsbDevice *dev, char *str, uint langId, uint strIndex)
{
    str[0] = '\0';
    if (!strIndex)
    {
        return true;
    }

    u8 buf[256];
    UsbStringDesc *desc = (UsbStringDesc *)buf;

    // Get string length
    if (!UsbDevRequest(dev,
        RT_DEV_TO_HOST | RT_STANDARD | RT_DEV,
        REQ_GET_DESC, (USB_DESC_STRING << 8) | strIndex, langId,
        1, desc))
    {
        return false;
    }

    // Get string data
    if (!UsbDevRequest(dev,
        RT_DEV_TO_HOST | RT_STANDARD | RT_DEV,
        REQ_GET_DESC, (USB_DESC_STRING << 8) | strIndex, langId,
        desc->len, desc))
    {
        return false;
    }

    // Dumb Unicode to ASCII conversion
    uint strLen = (desc->len - 2) / 2;
    for (uint i = 0; i < strLen; ++i)
    {
        str[i] = desc->str[i];
    }

    str[strLen] = '\0';
    return true;
}

// ------------------------------------------------------------------------------------------------
bool UsbDevClearHalt(UsbDevice *dev)
{
    return UsbDevRequest(dev,
        RT_DEV_TO_HOST | RT_STANDARD | RT_ENDP,
        REQ_CLEAR_FEATURE,
        F_ENDPOINT_HALT,
        dev->endp.desc.addr & 0xf,
        0, 0);
}
