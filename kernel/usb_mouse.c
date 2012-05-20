// ------------------------------------------------------------------------------------------------
// usb_mouse.c
// ------------------------------------------------------------------------------------------------

#include "usb_mouse.h"
#include "console.h"
#include "usb_class.h"

// ------------------------------------------------------------------------------------------------
static void usb_mouse_poll(USB_Device* dev)
{
    u8 data[4];

    if (dev->hc_poll(dev, 4, data))
    {
        console_print("%c%c%c dx=%d dy=%d\n",
            data[0] & 0x1 ? 'L' : ' ',
            data[0] & 0x2 ? 'R' : ' ',
            data[0] & 0x4 ? 'M' : ' ',
            (i8)data[1],
            (i8)data[2]);
    }
}

// ------------------------------------------------------------------------------------------------
bool usb_mouse_init(USB_Device* dev)
{
    if (dev->intf_desc.intf_class == USB_CLASS_HID &&
        dev->intf_desc.intf_subclass == USB_SUBCLASS_BOOT &&
        dev->intf_desc.intf_protocol == USB_PROTOCOL_MOUSE)
    {
        dev->drv_poll = usb_mouse_poll;
        return true;
    }
}
