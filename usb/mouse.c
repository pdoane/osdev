// ------------------------------------------------------------------------------------------------
// usb/mouse.c
// ------------------------------------------------------------------------------------------------

#include "usb/mouse.h"
#include "usb/registry.h"
#include "console/console.h"
#include "input/input.h"
#include "mem/vm.h"

// ------------------------------------------------------------------------------------------------
typedef struct UsbMouse
{
    UsbTransfer dataTransfer;
    u8 data[4];
} UsbMouse;

// ------------------------------------------------------------------------------------------------
static void UsbMouseProcess(UsbMouse *mouse)
{
    u8 *data = mouse->data;

    ConsolePrint("%c%c%c dx=%d dy=%d\n",
        data[0] & 0x1 ? 'L' : ' ',
        data[0] & 0x2 ? 'R' : ' ',
        data[0] & 0x4 ? 'M' : ' ',
        (i8)data[1],
        (i8)data[2]);

    InputOnMouse((i8)data[1], (i8)data[2]);
}

// ------------------------------------------------------------------------------------------------
static void UsbMousePoll(UsbDevice *dev)
{
    UsbMouse *mouse = dev->drv;
    UsbTransfer *t = &mouse->dataTransfer;

    if (t->complete)
    {
        if (t->success)
        {
            UsbMouseProcess(mouse);
        }

        t->complete = false;
        dev->hcIntr(dev, t);
    }
}

// ------------------------------------------------------------------------------------------------
bool UsbMouseInit(UsbDevice *dev)
{
    if (dev->intfDesc.intfClass == USB_CLASS_HID &&
        dev->intfDesc.intfSubClass == USB_SUBCLASS_BOOT &&
        dev->intfDesc.intfProtocol == USB_PROTOCOL_MOUSE)
    {
        ConsolePrint("Initializing Mouse\n");

        UsbMouse *mouse = VMAlloc(sizeof(UsbMouse));

        dev->drv = mouse;
        dev->drvPoll = UsbMousePoll;

        // Prepare transfer
        UsbTransfer *t = &mouse->dataTransfer;
        t->endp = &dev->endp;
        t->req = 0;
        t->data = mouse->data;
        t->len = 4;
        t->complete = false;
        t->success = false;

        dev->hcIntr(dev, t);
        return true;
    }

    return false;
}
