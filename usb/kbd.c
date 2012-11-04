// ------------------------------------------------------------------------------------------------
// usb/kbd.c
// ------------------------------------------------------------------------------------------------

#include "usb/kbd.h"
#include "usb/registry.h"
#include "console/console.h"
#include "input/input.h"
#include "input/keycode.h"
#include "mem/vm.h"
#include "stdlib/string.h"

// ------------------------------------------------------------------------------------------------
typedef struct UsbKbd
{
    UsbTransfer dataTransfer;
    u8 data[8];
    u8 lastData[8];
} UsbKbd;

// ------------------------------------------------------------------------------------------------
static void UsbKbdProcess(UsbKbd *kbd)
{
    u8 *data = kbd->data;
    bool error = false;

    // Modifier keys
    uint modDelta = data[0] ^ kbd->lastData[0];
    for (uint i = 0; i < 8; ++i)
    {
        uint mask = 1 << i;
        if (modDelta & mask)
        {
            InputOnKey(mask << 8, data[0] & mask);
        }
    }

    // Release keys
    for (uint i = 2; i < 8; ++i)
    {
        uint usage = kbd->lastData[i];

        if (usage)
        {
            if (!memchr(data + 2, usage, 6))
            {
                InputOnKey(usage, 0);
            }
        }
    }

    // Press keys
    for (uint i = 2; i < 8; ++i)
    {
        uint usage = data[i];

        if (usage >= 4)
        {
            if (!memchr(kbd->lastData + 2, usage, 6))
            {
                InputOnKey(usage, 1);
            }
        }
        else if (usage > 0)
        {
            error = true;
        }
    }

    // Update keystate
    if (!error)
    {
        memcpy(kbd->lastData, data, 8);
    }
}

// ------------------------------------------------------------------------------------------------
static void UsbKbdPoll(UsbDevice *dev)
{
    UsbKbd *kbd = dev->drv;

    UsbTransfer *t = &kbd->dataTransfer;

    if (t->complete)
    {
        if (t->success)
        {
            UsbKbdProcess(kbd);
        }

        t->complete = false;
        dev->hcIntr(dev, t);
    }
}

// ------------------------------------------------------------------------------------------------
bool UsbKbdInit(UsbDevice *dev)
{
    if (dev->intfDesc.intfClass == USB_CLASS_HID &&
        dev->intfDesc.intfSubClass == USB_SUBCLASS_BOOT &&
        dev->intfDesc.intfProtocol == USB_PROTOCOL_KBD)
    {
        ConsolePrint("Initializing Keyboard\n");

        UsbKbd *kbd = VMAlloc(sizeof(UsbKbd));
        memset(kbd->lastData, 0, 8);

        dev->drv = kbd;
        dev->drvPoll = UsbKbdPoll;

        uint intfIndex = dev->intfDesc.intfIndex;

        // Only send interrupt report when data changes
        UsbDevRequest(dev,
            RT_HOST_TO_DEV | RT_CLASS | RT_INTF,
            REQ_SET_IDLE, 0, intfIndex,
            0, 0);

        // Prepare transfer
        UsbTransfer *t = &kbd->dataTransfer;
        t->endp = &dev->endp;
        t->req = 0;
        t->data = kbd->data;
        t->len = 8;
        t->complete = false;
        t->success = false;

        dev->hcIntr(dev, t);
        return true;
    }

    return false;
}
