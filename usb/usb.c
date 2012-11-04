// ------------------------------------------------------------------------------------------------
// usb/usb.c
// ------------------------------------------------------------------------------------------------

#include "usb/usb.h"
#include "usb/controller.h"
#include "usb/dev.h"

// ------------------------------------------------------------------------------------------------
void UsbPoll()
{
    for (UsbController *c = g_usbControllerList; c; c = c->next)
    {
        if (c->poll)
        {
            c->poll(c);
        }
    }

    for (UsbDevice *dev = g_usbDeviceList; dev; dev = dev->next)
    {
        if (dev->drvPoll)
        {
            dev->drvPoll(dev);
        }
    }
}
