// ------------------------------------------------------------------------------------------------
// usb_kbd.c
// ------------------------------------------------------------------------------------------------

#include "usb_kbd.h"
#include "console.h"
#include "input.h"
#include "keycode.h"
#include "string.h"
#include "usb_class.h"
#include "vm.h"

// ------------------------------------------------------------------------------------------------
typedef struct USB_Kbd
{
    USB_Transfer data_transfer;
    u8 data[8];
    u8 last_data[8];
} USB_Kbd;

// ------------------------------------------------------------------------------------------------
static void usb_kbd_process(USB_Kbd* kbd)
{
    u8* data = kbd->data;
    bool error = false;

    // Modifier keys
    uint mod_delta = data[0] ^ kbd->last_data[0];
    for (uint i = 0; i < 8; ++i)
    {
        uint mask = 1 << i;
        if (mod_delta & mask)
        {
            input_event(mask << 8, data[0] & mask);
        }
    }

    // Release keys
    for (uint i = 2; i < 8; ++i)
    {
        uint usage = kbd->last_data[i];

        if (usage)
        {
            if (!memchr(data + 2, usage, 6))
            {
                input_event(usage, 0);
            }
        }
    }

    // Press keys
    for (uint i = 2; i < 8; ++i)
    {
        uint usage = data[i];

        if (usage >= 4)
        {
            if (!memchr(kbd->last_data + 2, usage, 6))
            {
                input_event(usage, 1);
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
        memcpy(kbd->last_data, data, 8);
    }
}

// ------------------------------------------------------------------------------------------------
static void usb_kbd_poll(USB_Device* dev)
{
    USB_Kbd* kbd = dev->drv;

    USB_Transfer* t = &kbd->data_transfer;

    if (t->complete)
    {
        if (t->success)
        {
            usb_kbd_process(kbd);
        }

        t->complete = false;
        dev->hc_intr(dev, t);
    }
}

// ------------------------------------------------------------------------------------------------
bool usb_kbd_init(USB_Device* dev)
{
    if (dev->intf_desc.intf_class == USB_CLASS_HID &&
        dev->intf_desc.intf_subclass == USB_SUBCLASS_BOOT &&
        dev->intf_desc.intf_protocol == USB_PROTOCOL_KBD)
    {
        console_print("Initializing Keyboard\n");

        USB_Kbd* kbd = vm_alloc(sizeof(USB_Kbd));
        memset(kbd->last_data, 0, 8);

        dev->drv = kbd;
        dev->drv_poll = usb_kbd_poll;

        uint intf_index = dev->intf_desc.intf_index;

        // Only send interrupt report when data changes
        usb_dev_request(dev,
            RT_HOST_TO_DEV | RT_CLASS | RT_INTF,
            REQ_SET_IDLE, 0, intf_index,
            0, 0);

        // Prepare transfer
        USB_Transfer* t = &kbd->data_transfer;
        t->endp = &dev->endp;
        t->req = 0;
        t->data = kbd->data;
        t->len = 8;
        t->complete = false;
        t->success = false;

        dev->hc_intr(dev, t);
        return true;
    }

    return false;
}
