// ------------------------------------------------------------------------------------------------
// usb/mouse.c
// ------------------------------------------------------------------------------------------------

#include "usb/mouse.h"
#include "usb/registry.h"
#include "console/console.h"
#include "mem/vm.h"

// ------------------------------------------------------------------------------------------------
typedef struct USB_Mouse
{
    USB_Transfer data_transfer;
    u8 data[4];
} USB_Mouse;

// ------------------------------------------------------------------------------------------------
static void usb_mouse_process(USB_Mouse* mouse)
{
    u8* data = mouse->data;

    console_print("%c%c%c dx=%d dy=%d\n",
        data[0] & 0x1 ? 'L' : ' ',
        data[0] & 0x2 ? 'R' : ' ',
        data[0] & 0x4 ? 'M' : ' ',
        (i8)data[1],
        (i8)data[2]);
}

// ------------------------------------------------------------------------------------------------
static void usb_mouse_poll(USB_Device* dev)
{
    USB_Mouse* mouse = dev->drv;
    USB_Transfer* t = &mouse->data_transfer;

    if (t->complete)
    {
        if (t->success)
        {
            usb_mouse_process(mouse);
        }

        t->complete = false;
        dev->hc_intr(dev, t);
    }
}

// ------------------------------------------------------------------------------------------------
bool usb_mouse_init(USB_Device* dev)
{
    if (dev->intf_desc.intf_class == USB_CLASS_HID &&
        dev->intf_desc.intf_subclass == USB_SUBCLASS_BOOT &&
        dev->intf_desc.intf_protocol == USB_PROTOCOL_MOUSE)
    {
        console_print("Initializing Mouse\n");

        USB_Mouse* mouse = vm_alloc(sizeof(USB_Mouse));

        dev->drv = mouse;
        dev->drv_poll = usb_mouse_poll;

        // Prepare transfer
        USB_Transfer* t = &mouse->data_transfer;
        t->endp = &dev->endp;
        t->req = 0;
        t->data = mouse->data;
        t->len = 4;
        t->complete = false;
        t->success = false;

        dev->hc_intr(dev, t);
        return true;
    }

    return false;
}
