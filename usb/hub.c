// ------------------------------------------------------------------------------------------------
// usb/hub.c
// ------------------------------------------------------------------------------------------------

#include "usb/hub.h"
#include "usb/registry.h"
#include "console/console.h"
#include "mem/vm.h"
#include "time/pit.h"

// ------------------------------------------------------------------------------------------------
// Port Status

#define PORT_CONNECTION                 (1 << 0)    // Current Connect Status
#define PORT_ENABLE                     (1 << 1)    // Port Enabled
#define PORT_SUSPEND                    (1 << 2)    // Suspend
#define PORT_OVER_CURRENT               (1 << 3)    // Over-current
#define PORT_RESET                      (1 << 4)    // Port Reset
#define PORT_POWER                      (1 << 8)    // Port Power
#define PORT_SPEED_MASK                 (3 << 9)    // Port Speed
#define PORT_SPEED_SHIFT                9
#define PORT_TEST                       (1 << 11)   // Port Test Control
#define PORT_INDICATOR                  (1 << 12)   // Port Indicator Control
#define PORT_CONNECTION_CHANGE          (1 << 16)   // Connect Status Change
#define PORT_ENABLE_CHANGE              (1 << 17)   // Port Enable Change
#define PORT_OVER_CURRENT_CHANGE        (1 << 19)   // Over-current Change

// ------------------------------------------------------------------------------------------------
typedef struct USB_Hub
{
    USB_Device* dev;
    USB_HubDesc desc;
} USB_Hub;

// ------------------------------------------------------------------------------------------------
static uint usb_hub_reset_port(USB_Hub* hub, uint port)
{
    USB_Device* dev = hub->dev;

    // Reset the port
    if (!usb_dev_request(dev,
        RT_HOST_TO_DEV | RT_CLASS | RT_OTHER,
        REQ_SET_FEATURE, F_PORT_RESET, port + 1,
        0, 0))
    {
        return 0;
    }

    // Wait 100ms for port to enable (TODO - remove after dynamic port detection)
    u32 status = 0;
    for (uint i = 0; i < 10; ++i)
    {
        // Delay
        pit_wait(10);

        // Get current status
        if (!usb_dev_request(dev,
            RT_DEV_TO_HOST | RT_CLASS | RT_OTHER,
            REQ_GET_STATUS, 0, port + 1,
            sizeof(status), &status))
        {
            return 0;
        }

        // Check if device is attached to port
        if (~status & PORT_CONNECTION)
        {
            break;
        }

        /*
        // Acknowledge change in status
        if (status & (PORT_ENABLE_CHANGE | PORT_CONNECTION_CHANGE))
        {
            port_clr(reg, PORT_ENABLE_CHANGE | PORT_CONNECTION_CHANGE);
            continue;
        }*/

        // Check if device is enabled
        if (status & PORT_ENABLE)
        {
            break;
        }
    }

    return status;
}

// ------------------------------------------------------------------------------------------------
static void usb_hub_probe(USB_Hub* hub)
{
    USB_Device* dev = hub->dev;
    uint port_count = hub->desc.port_count;

    // Enable power if needed
    if ((hub->desc.chars & HUB_POWER_MASK) == HUB_POWER_INDIVIDUAL)
    {
        for (uint port = 0; port < port_count; ++port)
        {
            if (!usb_dev_request(dev,
                RT_HOST_TO_DEV | RT_CLASS | RT_OTHER,
                REQ_SET_FEATURE, F_PORT_POWER, port + 1,
                0, 0))
            {
                return;
            }

        }

        pit_wait(hub->desc.port_power_time * 2);
    }

    // Reset ports
    for (uint port = 0; port < port_count; ++port)
    {
        uint status = usb_hub_reset_port(hub, port);

        if (status & PORT_ENABLE)
        {
            uint speed = (status & PORT_SPEED_MASK) >> PORT_SPEED_SHIFT;

            USB_Device* dev = usb_dev_create();
            if (dev)
            {
                dev->parent = hub->dev;
                dev->hc = hub->dev->hc;
                dev->port = port;
                dev->speed = speed;
                dev->max_packet_size = 8;

                dev->hc_control = hub->dev->hc_control;
                dev->hc_intr = hub->dev->hc_intr;

                if (!usb_dev_init(dev))
                {
                    // TODO - cleanup
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
static void usb_hub_poll(USB_Device* dev)
{
}

// ------------------------------------------------------------------------------------------------
bool usb_hub_init(USB_Device* dev)
{
    if (dev->intf_desc.intf_class == USB_CLASS_HUB)
    {
        console_print("Initializing Hub\n");

        // Get Hub Descriptor
        USB_HubDesc desc;

        if (!usb_dev_request(dev,
            RT_DEV_TO_HOST | RT_CLASS | RT_DEV,
            REQ_GET_DESC, (USB_DESC_HUB << 8) | 0, 0,
            sizeof(USB_HubDesc), &desc))
        {
            return false;
        }

        usb_print_hub_desc(&desc);

        USB_Hub* hub = vm_alloc(sizeof(USB_Hub));
        hub->dev = dev;
        hub->desc = desc;

        dev->drv = hub;
        dev->drv_poll = usb_hub_poll;

        usb_hub_probe(hub);
        return true;
    }

    return false;
}
