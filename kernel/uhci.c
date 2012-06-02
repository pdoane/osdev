// ------------------------------------------------------------------------------------------------
// uhci.c
// ------------------------------------------------------------------------------------------------

#include "uhci.h"
#include "console.h"
#include "io.h"
#include "pci_classify.h"
#include "pci_driver.h"
#include "pit.h"
#include "usb_dev.h"
#include "vm.h"

// ------------------------------------------------------------------------------------------------
// Limits

#define MAX_TRANSFER_DESCS              32

// ------------------------------------------------------------------------------------------------
// UHCI Controller I/O Registers

#define REG_CMD                         0x00        // USB Command
#define REG_STS                         0x02        // USB Status
#define REG_INTR                        0x04        // USB Interrupt Enable
#define REG_FRNUM                       0x06        // Frame Number
#define REG_FRBASEADD                   0x08        // Frame List Base Address
#define REG_SOFMOD                      0x0C        // Start of Frame Modify
#define REG_PORT1                       0x10        // Port 1 Status/Control
#define REG_PORT2                       0x12        // Port 2 Status/Control
#define REG_LEGSUP                      0xc0        // Legacy Support

// ------------------------------------------------------------------------------------------------
// USB Command Register

#define CMD_RS                          (1 << 0)    // Run/Stop
#define CMD_HCRESET                     (1 << 1)    // Host Controller Reset
#define CMD_GRESET                      (1 << 2)    // Global Reset
#define CMD_EGSM                        (1 << 3)    // Enter Global Suspend Resume
#define CMD_FGR                         (1 << 4)    // Force Global Resume
#define CMD_SWDBG                       (1 << 5)    // Software Debug
#define CMD_CF                          (1 << 6)    // Configure Flag
#define CMD_MAXP                        (1 << 7)    // Max Packet (0 = 32, 1 = 64)

// ------------------------------------------------------------------------------------------------
// USB Status Register

#define STS_USBINT                      (1 << 0)    // USB Interrupt
#define STS_ERROR                       (1 << 1)    // USB Error Interrupt
#define STS_RD                          (1 << 2)    // Resume Detect
#define STS_HSE                         (1 << 3)    // Host System Error
#define STS_HCPE                        (1 << 4)    // Host Controller Process Error
#define STS_HCH                         (1 << 5)    // HC Halted

// ------------------------------------------------------------------------------------------------
// USB Interrupt Enable Register

#define INTR_TIMEOUT                    (1 << 0)    // Timeout/CRC Interrupt Enable
#define INTR_RESUME                     (1 << 1)    // Resume Interrupt Enable
#define INTR_IOC                        (1 << 2)    // Interrupt on Complete Enable
#define INTR_SP                         (1 << 3)    // Short Packet Interrupt Enable

// ------------------------------------------------------------------------------------------------
// Port Status and Control Registers

#define PORT_CCS                        (1 << 0)    // Current Connect Status
#define PORT_CSC                        (1 << 1)    // Connect Status Change
#define PORT_PE                         (1 << 2)    // Port Enabled
#define PORT_PEC                        (1 << 3)    // Port Enable Change
#define PORT_LS                         (3 << 4)    // Line Status
#define PORT_RD                         (1 << 6)    // Resume Detect
#define PORT_LSDA                       (1 << 8)    // Low Speed Device Attached
#define PORT_RESET                      (1 << 9)    // Port Reset
#define PORT_SUSP                       (1 << 12)   // Suspend
#define PORT_RWC                        (PORT_PEC | PORT_CSC)

// ------------------------------------------------------------------------------------------------
// Transfer Descriptor

typedef struct UHCI_TD
{
    volatile u32 link;
    volatile u32 cs;
    volatile u32 token;
    volatile u32 buffer;
    u32 scratch[4];
} UHCI_TD;

// TD Link Pointer
#define TD_PTR_TERMINATE                (1 << 0)
#define TD_PTR_QH                       (1 << 1)
#define TD_PTR_DEPTH                    (1 << 2)

// TD Control and Status
#define TD_CS_ACTLEN                    0x000007ff
#define TD_CS_BITSTUFF                  (1 << 17)     // Bitstuff Error
#define TD_CS_CRC_TIMEOUT               (1 << 18)     // CRC/Time Out Error
#define TD_CS_NAK                       (1 << 19)     // NAK Received
#define TD_CS_BABBLE                    (1 << 20)     // Babble Detected
#define TD_CS_DATABUFFER                (1 << 21)     // Data Buffer Error
#define TD_CS_STALLED                   (1 << 22)     // Stalled
#define TD_CS_ACTIVE                    (1 << 23)     // Active
#define TD_CS_IOC                       (1 << 24)     // Interrupt on Complete
#define TD_CS_IOS                       (1 << 25)     // Isochronous Select
#define TD_CS_LOW_SPEED                 (1 << 26)     // Low Speed Device
#define TD_CS_ERROR_MASK                (3 << 27)     // Error counter
#define TD_CS_ERROR_SHIFT               27
#define TD_CS_SPD                       (1 << 29)     // Short Packet Detect

// TD Token
#define TD_TOK_PID_MASK                 0x000000ff    // Packet Identification
#define TD_TOK_DEVADDR_MASK             0x00007f00    // Device Address
#define TD_TOK_DEVADDR_SHIFT            8
#define TD_TOK_ENDP_MASK                00x0078000    // Endpoint
#define TD_TOK_ENDP_SHIFT               15
#define TD_TOK_D                        0x00080000    // Data Toggle
#define TD_TOK_D_SHIFT                  19
#define TD_TOK_MAXLEN_MASK              0xffe00000    // Maximum Length
#define TD_TOK_MAXLEN_SHIFT             21

#define TD_PACKET_IN                    0x69
#define TD_PACKET_OUT                   0xe1
#define TD_PACKET_SETUP                 0x2d

// ------------------------------------------------------------------------------------------------
// Queue Head

typedef struct UHCI_QH
{
    volatile u32 head;
    volatile u32 element;
    u8 pad[8];
} UHCI_QH;

// ------------------------------------------------------------------------------------------------
// Device

typedef struct UHCI_Controller
{
    uint io_addr;
    u32* frame_list;
    UHCI_QH* qh;
    UHCI_TD* td_pool;
} UHCI_Controller;

// ------------------------------------------------------------------------------------------------
static void uhci_port_set(uint port, u16 data)
{
    uint status = in16(port);
    status |= data;
    status &= ~PORT_RWC;
    out16(port, status);
}

// ------------------------------------------------------------------------------------------------
static void uhci_port_clr(uint port, u16 data)
{
    uint status = in16(port);
    status &= ~PORT_RWC;
    status &= ~data;
    status |= PORT_RWC & data;
    out16(port, status);
}

// ------------------------------------------------------------------------------------------------
static void uhci_td_init(UHCI_TD* td, UHCI_TD* prev,
                         uint speed, uint addr, uint endp, uint toggle, uint packet_type,
                         uint len, const void* data)
{
    len = (len - 1) & 0x7ff;

    if (prev)
    {
        prev->link = (u32)(uintptr_t)td | TD_PTR_DEPTH;
    }

    td->link = TD_PTR_TERMINATE;
    td->cs = (3 << TD_CS_ERROR_SHIFT) | TD_CS_ACTIVE;
    if (speed == USB_LOW_SPEED)
    {
        td->cs |= TD_CS_LOW_SPEED;
    }

    td->token =
        (len << TD_TOK_MAXLEN_SHIFT) |
        (toggle << TD_TOK_D_SHIFT) |
        (endp << TD_TOK_ENDP_SHIFT) |
        (addr << TD_TOK_DEVADDR_SHIFT) |
        packet_type;

    td->buffer = (u32)(uintptr_t)data;
}

// ------------------------------------------------------------------------------------------------
static bool uhci_qh_wait(UHCI_Controller* hc, UHCI_QH* qh)
{
    for (;;)
    {
        UHCI_TD* td = (UHCI_TD*)(uintptr_t)(qh->element & ~0xf);

        if (!td)
        {
            break;
        }

        if (~td->cs & TD_CS_ACTIVE)
        {
            if (td->cs & TD_CS_NAK)
            {
                console_print("NAK\n");
            }

            if (td->cs & TD_CS_STALLED)
            {
                console_print("TD is stalled\n");
                for (;;)
                {
                }
            }

            if (td->cs & TD_CS_DATABUFFER)
            {
                console_print("TD data buffer error\n");
            }
            if (td->cs & TD_CS_BABBLE)
            {
                console_print("TD babble error\n");
            }
            if (td->cs & TD_CS_CRC_TIMEOUT)
            {
                console_print("TD timeout error\n");
            }
            if (td->cs & TD_CS_BITSTUFF)
            {
                console_print("TD bitstuff error\n");
            }
        }
    }

    if (qh->element != TD_PTR_TERMINATE)
    {
        console_print("Queue not complete\n");
        for (;;)
        {
        }
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
static uint uhci_reset_port(UHCI_Controller* hc, uint port)
{
    uint reg = REG_PORT1 + port * 2;

    // Reset the port
    uhci_port_set(hc->io_addr + reg, PORT_RESET);
    pit_wait(50);
    uhci_port_clr(hc->io_addr + reg, PORT_RESET);

    // Wait 100ms for port to enable (TODO - what is appropriate length of time?)
    uint status = 0;
    for (uint i = 0; i < 10; ++i)
    {
        // Delay
        pit_wait(10);

        // Get current status
        status = in16(hc->io_addr + reg);

        // Check if device is attached to port
        if (~status & PORT_CCS)
        {
            break;
        }

        // Acknowledge change in status
        if (status & (PORT_PEC | PORT_CSC))
        {
            uhci_port_clr(hc->io_addr + reg, PORT_PEC | PORT_CSC);
            continue;
        }

        // Check if device is enabled
        if (status & PORT_PE)
        {
            break;
        }

        // Enable the port
        uhci_port_set(hc->io_addr + reg, PORT_PE);
    }

    return status;
}

// ------------------------------------------------------------------------------------------------
static bool uhci_dev_reset(USB_Device* dev)
{
    UHCI_Controller* hc = (UHCI_Controller*)dev->hc;

    uint status = uhci_reset_port(hc, dev->port);

    return status & PORT_PE;
}

// ------------------------------------------------------------------------------------------------
static bool uhci_dev_transfer(USB_Device* dev, USB_DevReq* req, void* data)
{
    UHCI_Controller* hc = (UHCI_Controller*)dev->hc;

    // Determine transfer properties
    uint speed = dev->speed;
    uint addr = dev->addr;
    uint endp = 0;
    uint max_size = dev->max_packet_size;
    uint type = req->type;
    uint len = req->len;

    // Create queue of transfer descriptors
    UHCI_TD* prev = 0;
    UHCI_TD* td = &hc->td_pool[0];

    // Setup packet
    uint toggle = 0;
    uint packet_type = TD_PACKET_SETUP;
    uint packet_size = sizeof(USB_DevReq);
    uhci_td_init(td, prev, speed, addr, endp, toggle, packet_type, packet_size, req);
    prev = td++;

    // Data in/out packets
    packet_type = type & RT_DEV_TO_HOST ? TD_PACKET_IN : TD_PACKET_OUT;

    u8* it = (u8*)data;
    u8* end = it + len;
    while (it < end)
    {
        // Check for overflow on data size
        uint td_index = td - hc->td_pool;
        if (td_index + 1 >= MAX_TRANSFER_DESCS)
        {
            console_print("USB Transfer too large %d %d\n", td_index, len);
            return false;
        }

        toggle ^= 1;
        packet_size = end - it;
        if (packet_size > max_size)
        {
            packet_size = max_size;
        }

        uhci_td_init(td, prev, speed, addr, endp, toggle, packet_type, packet_size, it);

        it += packet_size;
        prev = td++;
    }

    // Status packet
    toggle = 1;
    packet_type = type & RT_DEV_TO_HOST ? TD_PACKET_OUT : TD_PACKET_IN;
    uhci_td_init(td, prev, speed, addr, endp, toggle, packet_type, 0, 0);

    // Initialize queue head
    UHCI_QH* qh = &hc->qh[0];
    qh->element = (u32)(uintptr_t)&hc->td_pool[0];

    // Wait until queue has been processed
    return uhci_qh_wait(hc, qh);
}

// ------------------------------------------------------------------------------------------------
static bool uhci_dev_poll(USB_Device* dev, uint len, void* data)
{
    UHCI_Controller* hc = (UHCI_Controller*)dev->hc;

    // Determine transfer properties
    uint speed = dev->speed;
    uint addr = dev->addr;
    uint endp = dev->endp_desc.addr & 0xf;
    //uint max_size = dev->endp_desc.max_packet_size;

    // Create queue of transfer descriptors
    UHCI_TD* prev = 0;
    UHCI_TD* td = &hc->td_pool[0];

    // Data in/out packets
    uint toggle = dev->endp_toggle;
    uint packet_type = TD_PACKET_IN;
    uint packet_size = len;

    uhci_td_init(td, prev, speed, addr, endp, toggle, packet_type, packet_size, data);

    dev->endp_toggle ^= 1;

    // Initialize queue head
    UHCI_QH* qh = &hc->qh[0];
    qh->element = (u32)(uintptr_t)&hc->td_pool[0];

    // Wait until queue has been processed
    for (;;)
    {
        UHCI_TD* td = (UHCI_TD*)(uintptr_t)(qh->element & ~0xf);

        if (!td)
        {
            break;
        }
        else if (td->cs & TD_CS_NAK)
        {
            qh->element = TD_PTR_TERMINATE;
            return false;
        }
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
static void uhci_probe(UHCI_Controller* hc)
{
    // Port setup
    uint port_count = 2;    // TODO detect number of ports
    for (uint port = 0; port < port_count; ++port)
    {
        // Reset port
        uint status = uhci_reset_port(hc, port);

        if (status & PORT_PE)
        {
            uint speed = (status & PORT_LSDA) ? USB_LOW_SPEED : USB_FULL_SPEED;

            USB_Device* dev = usb_dev_create();
            if (dev)
            {
                dev->parent = 0;
                dev->hc = hc;
                dev->port = port;
                dev->speed = speed;
                dev->max_packet_size = 8;

                dev->hc_reset = uhci_dev_reset;
                dev->hc_transfer = uhci_dev_transfer;
                dev->hc_poll = uhci_dev_poll;

                if (!usb_dev_init(dev))
                {
                    // TODO - cleanup
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
void uhci_init(uint id, PCI_DeviceInfo* info)
{
    if (!(((info->class_code << 8) | info->subclass) == PCI_SERIAL_USB &&
        info->prog_intf == PCI_SERIAL_USB_UHCI))
    {
        return;
    }

    console_print ("Initializing UHCI\n");

    // Base I/O Address
    u32 bar4 = pci_in32 (id, PCI_CONFIG_BAR4);
    if (~bar4 & 1)
    {
        // Only Port I/O supported
        return;
    }

    uint io_addr = bar4 & ~0x3;

    // Controller initialization
    UHCI_Controller* hc = vm_alloc(sizeof(UHCI_Controller));
    hc->io_addr = io_addr;
    hc->frame_list = vm_alloc(1024 * sizeof(u32));
    hc->qh = (UHCI_QH*)vm_alloc(sizeof(UHCI_QH));
    hc->td_pool = (UHCI_TD*)vm_alloc(sizeof(UHCI_TD) * MAX_TRANSFER_DESCS);

    // Frame list setup
    hc->qh[0].head = TD_PTR_TERMINATE;
    hc->qh[0].element = TD_PTR_TERMINATE;

    for (uint i = 0; i < 1024; ++i)
    {
        hc->frame_list[i] = TD_PTR_QH | (u32)(uintptr_t)&hc->qh[0];
    }

    // Disable Legacy Support
    out16(hc->io_addr + REG_LEGSUP, 0x8f00);

    // Disable interrupts
    out16(hc->io_addr + REG_INTR, 0);

    // Assign frame list
    out16(hc->io_addr + REG_FRNUM, 0);
    out32(hc->io_addr + REG_FRBASEADD, (u32)(uintptr_t)hc->frame_list);
    out16(hc->io_addr + REG_SOFMOD, 0x40);

    // Clear status
    out16(hc->io_addr + REG_STS, 0xffff);

    // Enable controller
    out16(hc->io_addr + REG_CMD, CMD_RS);

    // Probe devices
    uhci_probe(hc);
}
