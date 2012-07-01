// ------------------------------------------------------------------------------------------------
// usb/uhci.c
// ------------------------------------------------------------------------------------------------

#include "usb/uhci.h"
#include "usb/controller.h"
#include "usb/dev.h"
#include "console/console.h"
#include "cpu/io.h"
#include "pci/driver.h"
#include "pci/registry.h"
#include "mem/vm.h"
#include "stdlib/link.h"
#include "stdlib/string.h"
#include "time/pit.h"

// ------------------------------------------------------------------------------------------------
// Limits

#define MAX_QH                          8
#define MAX_TD                          32

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

#define PORT_CONNECTION                 (1 << 0)    // Current Connect Status
#define PORT_CONNECTION_CHANGE          (1 << 1)    // Connect Status Change
#define PORT_ENABLE                     (1 << 2)    // Port Enabled
#define PORT_ENABLE_CHANGE              (1 << 3)    // Port Enable Change
#define PORT_LS                         (3 << 4)    // Line Status
#define PORT_RD                         (1 << 6)    // Resume Detect
#define PORT_LSDA                       (1 << 8)    // Low Speed Device Attached
#define PORT_RESET                      (1 << 9)    // Port Reset
#define PORT_SUSP                       (1 << 12)   // Suspend
#define PORT_RWC                        (PORT_CONNECTION_CHANGE | PORT_ENABLE_CHANGE)

// ------------------------------------------------------------------------------------------------
// Transfer Descriptor

typedef struct UHCI_TD
{
    volatile u32 link;
    volatile u32 cs;
    volatile u32 token;
    volatile u32 buffer;

    // internal fields
    u32 td_next;
    u8 active;
    u8 pad[11];
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

    // internal fields
    USB_Transfer* transfer;
    Link qh_link;
    u32 td_head;
    u32 active;
    u8 pad[24];
} UHCI_QH;

// ------------------------------------------------------------------------------------------------
// Device

typedef struct UHCI_Controller
{
    uint io_addr;
    u32* frame_list;
    UHCI_QH* qh_pool;
    UHCI_TD* td_pool;
    UHCI_QH* async_qh;
} UHCI_Controller;

#if 0
// ------------------------------------------------------------------------------------------------
static void uhci_print_td(UHCI_TD* td)
{
    console_print("td=0x%08x: link=0x%08x cs=0x%08x token=0x%08x buffer=0x%08x\n",
            td, td->link, td->cs, td->token, td->buffer);
}

// ------------------------------------------------------------------------------------------------
static void uhci_print_qh(UHCI_QH* qh)
{
    console_print("qh=0x%08x: head=0x%08x element=0x%08x\n",
            qh, qh->head, qh->element);
}
#endif

// ------------------------------------------------------------------------------------------------
static UHCI_TD* uhci_alloc_td(UHCI_Controller* hc)
{
    // TODO - better memory management
    UHCI_TD* end = hc->td_pool + MAX_TD;
    for (UHCI_TD* td = hc->td_pool; td != end; ++td)
    {
        if (!td->active)
        {
            //console_print("uhci_alloc_td 0x%08x\n", td);
            td->active = 1;
            return td;
        }
    }

    console_print("uhci_alloc_td failed\n");
    return 0;
}

// ------------------------------------------------------------------------------------------------
static UHCI_QH* uhci_alloc_qh(UHCI_Controller* hc)
{
    // TODO - better memory management
    UHCI_QH* end = hc->qh_pool + MAX_QH;
    for (UHCI_QH* qh = hc->qh_pool; qh != end; ++qh)
    {
        if (!qh->active)
        {
            //console_print("uhci_alloc_qh 0x%08x\n", qh);
            qh->active = 1;
            return qh;
        }
    }

    console_print("uhci_alloc_qh failed\n");
    return 0;
}

// ------------------------------------------------------------------------------------------------
static void uhci_free_td(UHCI_TD* td)
{
    //console_print("uhci_free_td 0x%08x\n", td);
    td->active = 0;
}

// ------------------------------------------------------------------------------------------------
static void uhci_free_qh(UHCI_QH* qh)
{
    //console_print("uhci_free_qh 0x%08x\n", qh);
    qh->active = 0;
}

// ------------------------------------------------------------------------------------------------
static void uhci_insert_qh(UHCI_Controller* hc, UHCI_QH* qh)
{
    UHCI_QH* list = hc->async_qh;
    UHCI_QH* end = link_data(list->qh_link.prev, UHCI_QH, qh_link);

    qh->head = TD_PTR_TERMINATE;
    end->head = (u32)(uintptr_t)qh | TD_PTR_QH;

    link_before(&list->qh_link, &qh->qh_link);
}

// ------------------------------------------------------------------------------------------------
static void uhci_remove_qh(UHCI_QH* qh)
{
    UHCI_QH* prev = link_data(qh->qh_link.prev, UHCI_QH, qh_link);

    prev->head = qh->head;
    link_remove(&qh->qh_link);
}

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
        prev->td_next = (u32)(uintptr_t)td;
    }

    td->link = TD_PTR_TERMINATE;
    td->td_next = 0;

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
static void uhci_qh_init(UHCI_QH* qh, USB_Transfer* t, UHCI_TD* td)
{
    qh->transfer = t;
    qh->td_head = (u32)(uintptr_t)td;
    qh->element = (u32)(uintptr_t)td;
}

// ------------------------------------------------------------------------------------------------
static void uhci_qh_process(UHCI_Controller* hc, UHCI_QH* qh)
{
    USB_Transfer* t = qh->transfer;

    UHCI_TD* td = (UHCI_TD*)(uintptr_t)(qh->element & ~0xf);
    if (!td)
    {
        t->success = true;
        t->complete = true;
    }
    else if (~td->cs & TD_CS_ACTIVE)
    {
        if (td->cs & TD_CS_NAK)
        {
            console_print("NAK\n");
        }

        if (td->cs & TD_CS_STALLED)
        {
            console_print("TD is stalled\n");
            t->success = false;
            t->complete = true;
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

    if (t->complete)
    {
        // Clear transfer from queue
        qh->transfer = 0;

        // Update endpoint toggle state
        if (t->success && t->endp)
        {
            t->endp->toggle ^= 1;
        }

        // Remove queue from schedule
        uhci_remove_qh(qh);

        // Free transfer descriptors
        UHCI_TD* td = (UHCI_TD*)(uintptr_t)qh->td_head;
        while (td)
        {
            UHCI_TD* next = (UHCI_TD*)(uintptr_t)td->td_next;
            uhci_free_td(td);
            td = next;
        }

        // Free queue head
        uhci_free_qh(qh);
    }
}

// ------------------------------------------------------------------------------------------------
static void uhci_qh_wait(UHCI_Controller* hc, UHCI_QH* qh)
{
    USB_Transfer* t = qh->transfer;

    while (!t->complete)
    {
        uhci_qh_process(hc, qh);
    }
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
        if (~status & PORT_CONNECTION)
        {
            break;
        }

        // Acknowledge change in status
        if (status & (PORT_ENABLE_CHANGE | PORT_CONNECTION_CHANGE))
        {
            uhci_port_clr(hc->io_addr + reg, PORT_ENABLE_CHANGE | PORT_CONNECTION_CHANGE);
            continue;
        }

        // Check if device is enabled
        if (status & PORT_ENABLE)
        {
            break;
        }

        // Enable the port
        uhci_port_set(hc->io_addr + reg, PORT_ENABLE);
    }

    return status;
}

// ------------------------------------------------------------------------------------------------
static void uhci_dev_control(USB_Device* dev, USB_Transfer* t)
{
    UHCI_Controller* hc = (UHCI_Controller*)dev->hc;
    USB_DevReq* req = t->req;

    // Determine transfer properties
    uint speed = dev->speed;
    uint addr = dev->addr;
    uint endp = 0;
    uint max_size = dev->max_packet_size;
    uint type = req->type;
    uint len = req->len;

    // Create queue of transfer descriptors
    UHCI_TD* td = uhci_alloc_td(hc);
    if (!td)
    {
        return;
    }

    UHCI_TD* head = td;
    UHCI_TD* prev = 0;

    // Setup packet
    uint toggle = 0;
    uint packet_type = TD_PACKET_SETUP;
    uint packet_size = sizeof(USB_DevReq);
    uhci_td_init(td, prev, speed, addr, endp, toggle, packet_type, packet_size, req);
    prev = td;

    // Data in/out packets
    packet_type = type & RT_DEV_TO_HOST ? TD_PACKET_IN : TD_PACKET_OUT;

    u8* it = (u8*)t->data;
    u8* end = it + len;
    while (it < end)
    {
        td = uhci_alloc_td(hc);
        if (!td)
        {
            return;
        }

        toggle ^= 1;
        packet_size = end - it;
        if (packet_size > max_size)
        {
            packet_size = max_size;
        }

        uhci_td_init(td, prev, speed, addr, endp, toggle, packet_type, packet_size, it);

        it += packet_size;
        prev = td;
    }

    // Status packet
    td = uhci_alloc_td(hc);
    if (!td)
    {
        return;
    }

    toggle = 1;
    packet_type = type & RT_DEV_TO_HOST ? TD_PACKET_OUT : TD_PACKET_IN;
    uhci_td_init(td, prev, speed, addr, endp, toggle, packet_type, 0, 0);

    // Initialize queue head
    UHCI_QH* qh = uhci_alloc_qh(hc);
    uhci_qh_init(qh, t, head);

    // Wait until queue has been processed
    uhci_insert_qh(hc, qh);
    uhci_qh_wait(hc, qh);
}

// ------------------------------------------------------------------------------------------------
static void uhci_dev_intr(USB_Device* dev, USB_Transfer* t)
{
    UHCI_Controller* hc = (UHCI_Controller*)dev->hc;

    // Determine transfer properties
    uint speed = dev->speed;
    uint addr = dev->addr;
    uint endp = dev->endp.desc.addr & 0xf;

    // Create queue of transfer descriptors
    UHCI_TD* td = uhci_alloc_td(hc);
    if (!td)
    {
        t->success = false;
        t->complete = true;
        return;
    }

    UHCI_TD* head = td;
    UHCI_TD* prev = 0;

    // Data in/out packets
    uint toggle = dev->endp.toggle;
    uint packet_type = TD_PACKET_IN;
    uint packet_size = t->len;

    uhci_td_init(td, prev, speed, addr, endp, toggle, packet_type, packet_size, t->data);

    // Initialize queue head
    UHCI_QH* qh = uhci_alloc_qh(hc);
    uhci_qh_init(qh, t, head);

    // Schedule queue
    uhci_insert_qh(hc, qh);
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

        if (status & PORT_ENABLE)
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

                dev->hc_control = uhci_dev_control;
                dev->hc_intr = uhci_dev_intr;

                if (!usb_dev_init(dev))
                {
                    // TODO - cleanup
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
static void uhci_controller_poll(USB_Controller* controller)
{
    UHCI_Controller* hc = (UHCI_Controller*)controller->hc;

    UHCI_QH* qh = link_data(hc->async_qh->qh_link.next, UHCI_QH, qh_link);
    UHCI_QH* end = hc->async_qh;
    while (qh != end)
    {
        UHCI_QH* next = link_data(qh->qh_link.next, UHCI_QH, qh_link);
        if (qh->transfer)
        {
            uhci_qh_process(hc, qh);
        }

        qh = next;
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
    PCI_Bar bar;
    pci_get_bar(&bar, id, 4);
    if (~bar.flags & PCI_BAR_IO)
    {
        // Only Port I/O supported
        return;
    }

    uint io_addr = bar.u.port;

    // Controller initialization
    UHCI_Controller* hc = vm_alloc(sizeof(UHCI_Controller));
    hc->io_addr = io_addr;
    hc->frame_list = vm_alloc(1024 * sizeof(u32));
    hc->qh_pool = (UHCI_QH*)vm_alloc(sizeof(UHCI_QH) * MAX_QH);
    hc->td_pool = (UHCI_TD*)vm_alloc(sizeof(UHCI_TD) * MAX_TD);

    memset(hc->qh_pool, 0, sizeof(UHCI_QH) * MAX_QH);
    memset(hc->td_pool, 0, sizeof(UHCI_TD) * MAX_TD);

    // Frame list setup
    UHCI_QH* qh = uhci_alloc_qh(hc);
    qh->head = TD_PTR_TERMINATE;
    qh->element = TD_PTR_TERMINATE;
    qh->transfer = 0;
    qh->qh_link.prev = &qh->qh_link;
    qh->qh_link.next = &qh->qh_link;

    hc->async_qh = qh;
    for (uint i = 0; i < 1024; ++i)
    {
        hc->frame_list[i] = TD_PTR_QH | (u32)(uintptr_t)qh;
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

    // Register controller
    USB_Controller* controller = (USB_Controller*)vm_alloc(sizeof(USB_Controller));
    controller->next = g_usb_controller_list;
    controller->hc = hc;
    controller->poll = uhci_controller_poll;

    g_usb_controller_list = controller;
}
