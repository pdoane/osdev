// ------------------------------------------------------------------------------------------------
// usb/desc.c
// ------------------------------------------------------------------------------------------------

#include "usb/desc.h"
#include "console/console.h"

// ------------------------------------------------------------------------------------------------
void usb_print_device_desc(USB_DeviceDesc* desc)
{
    console_print(" USB: Version=%d.%d Vendor ID=%04x Product ID=%04x Configs=%d\n",
        desc->usb_ver >> 8, (desc->usb_ver >> 4) & 0xf,
        desc->vendor_id, desc->product_id,
        desc->conf_count);
}

// ------------------------------------------------------------------------------------------------
void usb_print_conf_desc(USB_ConfDesc* desc)
{
    console_print("  Conf: total_len=%d intf_count=%d conf_value=%d conf_str=%d\n",
        desc->total_len,
        desc->intf_count,
        desc->conf_value,
        desc->conf_str);
}

// ------------------------------------------------------------------------------------------------
void usb_print_intf_desc(USB_IntfDesc* desc)
{
    console_print("  Intf: alt_setting=%d endp_count=%d class=%d subclass=%d protocol=%d str=%d\n",
        desc->alt_setting,
        desc->endp_count,
        desc->intf_class,
        desc->intf_subclass,
        desc->intf_protocol,
        desc->intf_str);
}

// ------------------------------------------------------------------------------------------------
void usb_print_endp_desc(USB_EndpDesc* desc)
{
    console_print("  Endp: addr=0x%02x attributes=%d max_packet_size=%d interval=%d\n",
        desc->addr,
        desc->attributes,
        desc->max_packet_size,
        desc->interval);
}

// ------------------------------------------------------------------------------------------------
void usb_print_hid_desc(USB_HidDesc* desc)
{
    console_print("  HID: ver=%d.%d country=%d desc_count=%d desc_type=%x desc_len=%d\n",
        desc->hid_ver >> 8, (desc->hid_ver >> 8) & 0xff,
        desc->country_code,
        desc->desc_count,
        desc->desc_type,
        desc->desc_len);
}

// ------------------------------------------------------------------------------------------------
void usb_print_hub_desc(USB_HubDesc* desc)
{
    console_print(" Hub: port count=%d characteristics=0x%x power time=%d current=%d\n",
            desc->port_count,
            desc->chars,
            desc->port_power_time,
            desc->current);
}
