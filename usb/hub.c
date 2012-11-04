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
typedef struct UsbHub
{
    UsbDevice *dev;
    UsbHubDesc desc;
} UsbHub;

// ------------------------------------------------------------------------------------------------
static uint UsbHubResetPort(UsbHub *hub, uint port)
{
    UsbDevice *dev = hub->dev;

    // Reset the port
    if (!UsbDevRequest(dev,
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
        PitWait(10);

        // Get current status
        if (!UsbDevRequest(dev,
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
static void UsbHubProbe(UsbHub *hub)
{
    UsbDevice *dev = hub->dev;
    uint portCount = hub->desc.portCount;

    // Enable power if needed
    if ((hub->desc.chars & HUB_POWER_MASK) == HUB_POWER_INDIVIDUAL)
    {
        for (uint port = 0; port < portCount; ++port)
        {
            if (!UsbDevRequest(dev,
                RT_HOST_TO_DEV | RT_CLASS | RT_OTHER,
                REQ_SET_FEATURE, F_PORT_POWER, port + 1,
                0, 0))
            {
                return;
            }

        }

        PitWait(hub->desc.portPowerTime * 2);
    }

    // Reset ports
    for (uint port = 0; port < portCount; ++port)
    {
        uint status = UsbHubResetPort(hub, port);

        if (status & PORT_ENABLE)
        {
            uint speed = (status & PORT_SPEED_MASK) >> PORT_SPEED_SHIFT;

            UsbDevice *dev = UsbDevCreate();
            if (dev)
            {
                dev->parent = hub->dev;
                dev->hc = hub->dev->hc;
                dev->port = port;
                dev->speed = speed;
                dev->maxPacketSize = 8;

                dev->hcControl = hub->dev->hcControl;
                dev->hcIntr = hub->dev->hcIntr;

                if (!UsbDevInit(dev))
                {
                    // TODO - cleanup
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
static void UsbHubPoll(UsbDevice *dev)
{
}

// ------------------------------------------------------------------------------------------------
bool UsbHubInit(UsbDevice *dev)
{
    if (dev->intfDesc.intfClass == USB_CLASS_HUB)
    {
        ConsolePrint("Initializing Hub\n");

        // Get Hub Descriptor
        UsbHubDesc desc;

        if (!UsbDevRequest(dev,
            RT_DEV_TO_HOST | RT_CLASS | RT_DEV,
            REQ_GET_DESC, (USB_DESC_HUB << 8) | 0, 0,
            sizeof(UsbHubDesc), &desc))
        {
            return false;
        }

        UsbPrintHubDesc(&desc);

        UsbHub *hub = VMAlloc(sizeof(UsbHub));
        hub->dev = dev;
        hub->desc = desc;

        dev->drv = hub;
        dev->drvPoll = UsbHubPoll;

        UsbHubProbe(hub);
        return true;
    }

    return false;
}
