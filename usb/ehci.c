// ------------------------------------------------------------------------------------------------
// usb/ehci.c
// ------------------------------------------------------------------------------------------------

#include "usb/ehci.h"
#include "usb/controller.h"
#include "usb/dev.h"
#include "console/console.h"
#include "cpu/io.h"
#include "mem/vm.h"
#include "pci/registry.h"
#include "pci/driver.h"
#include "time/pit.h"
#include "stdlib/link.h"
#include "stdlib/string.h"

// ------------------------------------------------------------------------------------------------
// Limits

#define MAX_QH                          8
#define MAX_TD                          32

// ------------------------------------------------------------------------------------------------
// PCI Configuration Registers

// EECP-based
#define USBLEGSUP                       0x00        // USB Legacy Support Extended Capability
#define UBSLEGCTLSTS                    0x04        // USB Legacy Support Control/Status

// ------------------------------------------------------------------------------------------------
// USB Legacy Support Register

#define USBLEGSUP_HC_OS                 0x01000000  // HC OS Owned Semaphore
#define USBLEGSUP_HC_BIOS               0x00010000  // HC BIOS Owned Semaphore
#define USBLEGSUP_NCP_MASK              0x0000ff00  // Next EHCI Extended Capability Pointer
#define USBLEGSUP_CAPID                 0x000000ff  // Capability ID

// ------------------------------------------------------------------------------------------------
// Host Controller Capability Registers

typedef struct EHCI_Cap_Regs
{
    u8 cap_length;
    u8 reserved;
    u16 hci_version;
    u32 hcs_params;
    u32 hcc_params;
    u64 hcsp_portroute;
} PACKED EHCI_Cap_Regs;

// ------------------------------------------------------------------------------------------------
// Host Controller Structural Parameters Register

#define HCSPARAMS_N_PORTS_MASK          (15 << 0)   // Number of Ports
#define HCSPARAMS_PPC                   (1 << 4)    // Port Power Control
#define HCSPARAMS_PORT_ROUTE            (1 << 7)    // Port Routing Rules
#define HCSPARAMS_N_PCC_MASK            (15 << 8)   // Number of Ports per Companion Controller
#define HCSPARAMS_N_PCC_SHIFT           8
#define HCSPARAMS_N_CC_MASK             (15 << 12)  // Number of Companion Controllers
#define HCSPARAMS_N_CC_SHIFT            12
#define HCSPARAMS_P_INDICATOR           (1 << 16)   // Port Indicator
#define HCSPARAMS_DPN_MASK              (15 << 20)  // Debug Port Number
#define HCSPARAMS_DPN_SHIFT             20

// ------------------------------------------------------------------------------------------------
// Host Controller Capability Parameters Register

#define HCCPARAMS_64_BIT                (1 << 0)    // 64-bit Addressing Capability
#define HCCPARAMS_PFLF                  (1 << 1)    // Programmable Frame List Flag
#define HCCPARAMS_ASPC                  (1 << 2)    // Asynchronous Schedule Park Capability
#define HCCPARAMS_IST_MASK              (15 << 4)   // Isochronous Sheduling Threshold
#define HCCPARAMS_EECP_MASK             (255 << 8)  // EHCI Extended Capabilities Pointer
#define HCCPARAMS_EECP_SHIFT            8

// ------------------------------------------------------------------------------------------------
// Host Controller Operational Registers

typedef struct EHCI_Op_Regs
{
    volatile u32 usb_cmd;
    volatile u32 usb_sts;
    volatile u32 usb_intr;
    volatile u32 frame_index;
    volatile u32 ctrl_ds_segment;
    volatile u32 periodic_list_base;
    volatile u32 async_list_addr;
    volatile u32 reserved[9];
    volatile u32 config_flag;
    volatile u32 ports[];
} EHCI_Op_Regs;

// ------------------------------------------------------------------------------------------------
// USB Command Register

#define CMD_RS                          (1 << 0)    // Run/Stop
#define CMD_HCRESET                     (1 << 1)    // Host Controller Reset
#define CMD_FLS_MASK                    (3 << 2)    // Frame List Size
#define CMD_FLS_SHIFT                   2
#define CMD_PSE                         (1 << 4)    // Periodic Schedule Enable
#define CMD_ASE                         (1 << 5)    // Asynchronous Schedule Enable
#define CMD_IOAAD                       (1 << 6)    // Interrupt on Async Advance Doorbell
#define CMD_LHCR                        (1 << 7)    // Light Host Controller Reset
#define CMD_ASPMC_MASK                  (3 << 8)    // Asynchronous Schedule Park Mode Count
#define CMD_ASPMC_SHIFT                 8
#define CMD_ASPME                       (1 << 11)   // Asynchronous Schedule Park Mode Enable
#define CMD_ITC_MASK                    (255 << 16) // Interrupt Threshold Control
#define CMD_ITC_SHIFT                   16

// ------------------------------------------------------------------------------------------------
// USB Status Register

#define STS_USBINT                      (1 << 0)    // USB Interrupt
#define STS_ERROR                       (1 << 1)    // USB Error Interrupt
#define STS_PCD                         (1 << 2)    // Port Change Detect
#define STS_FLR                         (1 << 3)    // Frame List Rollover
#define STS_HSE                         (1 << 4)    // Host System Error
#define STS_IOAA                        (1 << 5)    // Interrupt on Async Advance
#define STS_HCHALTED                    (1 << 12)   // Host Controller Halted
#define STS_RECLAMATION                 (1 << 13)   // Reclamation
#define STS_PSS                         (1 << 14)   // Periodic Schedule Status
#define STS_ASS                         (1 << 15)   // Asynchronous Schedule Status

// ------------------------------------------------------------------------------------------------
// USB Interrupt Enable Register

#define INTR_USBINT                     (1 << 0)    // USB Interrupt Enable
#define INTR_ERROR                      (1 << 1)    // USB Error Interrupt Enable
#define INTR_PCD                        (1 << 2)    // Port Change Interrupt Enable
#define INTR_FLR                        (1 << 3)    // Frame List Rollover Enable
#define INTR_HSE                        (1 << 4)    // Host System Error Enable
#define INTR_IOAA                       (1 << 5)    // Interrupt on Async Advance Enable

// ------------------------------------------------------------------------------------------------
// Frame Index Register

#define FR_MASK                         0x3fff

// ------------------------------------------------------------------------------------------------
// Configure Flag Register

#define CF_PORT_ROUTE                   (1 << 0)    // Configure Flag (CF)

// ------------------------------------------------------------------------------------------------
// Port Status and Control Registers

#define PORT_CONNECTION                 (1 << 0)    // Current Connect Status
#define PORT_CONNECTION_CHANGE          (1 << 1)    // Connect Status Change
#define PORT_ENABLE                     (1 << 2)    // Port Enabled
#define PORT_ENABLE_CHANGE              (1 << 3)    // Port Enable Change
#define PORT_OVER_CURRENT               (1 << 4)    // Over-current Active
#define PORT_OVER_CURRENT_CHANGE        (1 << 5)    // Over-current Change
#define PORT_FPR                        (1 << 6)    // Force Port Resume
#define PORT_SUSPEND                    (1 << 7)    // Suspend
#define PORT_RESET                      (1 << 8)    // Port Reset
#define PORT_LS_MASK                    (3 << 10)   // Line Status
#define PORT_LS_SHIFT                   10
#define PORT_POWER                      (1 << 12)   // Port Power
#define PORT_OWNER                      (1 << 13)   // Port Owner
#define PORT_IC_MASK                    (3 << 14)   // Port Indicator Control
#define PORT_IC_SHIFT                   14
#define PORT_TC_MASK                    (15 << 16)  // Port Test Control
#define PORT_TC_SHIFT                   16
#define PORT_WKCNNT_E                   (1 << 20)   // Wake on Connect Enable
#define PORT_WKDSCNNT_E                 (1 << 21)   // Wake on Disconnect Enable
#define PORT_WKOC_E                     (1 << 22)   // Wake on Over-current Enable
#define PORT_RWC                        (PORT_CONNECTION_CHANGE | PORT_ENABLE_CHANGE | PORT_OVER_CURRENT_CHANGE)

// ------------------------------------------------------------------------------------------------
// Transfer Descriptor

typedef struct EHCI_TD
{
    volatile u32 link;
    volatile u32 alt_link;
    volatile u32 token;
    volatile u32 buffer[5];
    volatile u32 ext_buffer[5];

    // internal fields
    u32 td_next;
    u32 active;
    u8 pad[4];
} EHCI_TD;

// TD Link Pointer
#define PTR_TERMINATE                   (1 << 0)

#define PTR_TYPE_MASK                   (3 << 1)
#define PTR_ITD                         (0 << 1)
#define PTR_QH                          (1 << 1)
#define PTR_SITD                        (2 << 1)
#define PTR_FSTN                        (3 << 1)

// TD Token
#define TD_TOK_PING                     (1 << 0)    // Ping State
#define TD_TOK_STS                      (1 << 1)    // Split Transaction State
#define TD_TOK_MMF                      (1 << 2)    // Missed Micro-Frame
#define TD_TOK_XACT                     (1 << 3)    // Transaction Error
#define TD_TOK_BABBLE                   (1 << 4)    // Babble Detected
#define TD_TOK_DATABUFFER               (1 << 5)    // Data Buffer Error
#define TD_TOK_HALTED                   (1 << 6)    // Halted
#define TD_TOK_ACTIVE                   (1 << 7)    // Active
#define TD_TOK_PID_MASK                 (3 << 8)    // PID Code
#define TD_TOK_PID_SHIFT                8
#define TD_TOK_CERR_MASK                (3 << 10)   // Error Counter
#define TD_TOK_CERR_SHIFT               10
#define TD_TOK_C_PAGE_MASK              (7 << 12)   // Current Page
#define TD_TOK_C_PAGE_SHIFT             12
#define TD_TOK_IOC                      (1 << 15)   // Interrupt on Complete
#define TD_TOK_LEN_MASK                 0x7fff0000  // Total Bytes to Transfer
#define TD_TOK_LEN_SHIFT                16
#define TD_TOK_D                        (1 << 31)   // Data Toggle
#define TD_TOK_D_SHIFT                  31

#define USB_PACKET_OUT                  0           // token 0xe1
#define USB_PACKET_IN                   1           // token 0x69
#define USB_PACKET_SETUP                2           // token 0x2d

// ------------------------------------------------------------------------------------------------
// Queue Head

typedef struct EHCI_QH
{
    u32 qhlp;       // Queue Head Horizontal Link Pointer
    u32 ch;         // Endpoint Characteristics
    u32 caps;       // Endpoint Capabilities
    volatile u32 cur_link;

    // matches a transfer descriptor
    volatile u32 next_link;
    volatile u32 alt_link;
    volatile u32 token;
    volatile u32 buffer[5];
    volatile u32 ext_buffer[5];

    // internal fields
    USB_Transfer* transfer;
    Link qh_link;
    u32 td_head;
    u32 active;
    u8 pad[20];
} EHCI_QH;

// Endpoint Characteristics
#define QH_CH_DEVADDR_MASK              0x0000007f  // Device Address
#define QH_CH_INACTIVE                  0x00000080  // Inactive on Next Transaction
#define QH_CH_ENDP_MASK                 0x00000f00  // Endpoint Number
#define QH_CH_ENDP_SHIFT                8
#define QH_CH_EPS_MASK                  0x00003000  // Endpoint Speed
#define QH_CH_EPS_SHIFT                 12
#define QH_CH_DTC                       0x00004000  // Data Toggle Control
#define QH_CH_H                         0x00008000  // Head of Reclamation List Flag
#define QH_CH_MPL_MASK                  0x07ff0000  // Maximum Packet Length
#define QH_CH_MPL_SHIFT                 16
#define QH_CH_CONTROL                   0x08000000  // Control Endpoint Flag
#define QH_CH_NAK_RL_MASK               0xf0000000  // Nak Count Reload
#define QH_CH_NAK_RL_SHIFT              28

// Endpoint Capabilities
#define QH_CAP_INT_SCHED_SHIFT          0
#define QH_CAP_INT_SCHED_MASK           0x000000ff  // Interrupt Schedule Mask
#define QH_CAP_SPLIT_C_SHIFT            8
#define QH_CAP_SPLIT_C_MASK             0x0000ff00  // Split Completion Mask
#define QH_CAP_HUB_ADDR_SHIFT           16
#define QH_CAP_HUB_ADDR_MASK            0x007f0000  // Hub Address
#define QH_CAP_PORT_MASK                0x3f800000  // Port Number
#define QH_CAP_PORT_SHIFT               23
#define QH_CAP_MULT_MASK                0xc0000000  // High-Bandwidth Pipe Multiplier
#define QH_CAP_MULT_SHIFT               30

// ------------------------------------------------------------------------------------------------
// Device

typedef struct EHCI_Controller
{
    EHCI_Cap_Regs* cap_regs;
    EHCI_Op_Regs* op_regs;
    u32* frame_list;
    EHCI_QH* qh_pool;
    EHCI_TD* td_pool;
    EHCI_QH* async_qh;
    EHCI_QH* periodic_qh;
} EHCI_Controller;

#if 0
// ------------------------------------------------------------------------------------------------
static void ehci_print_td(EHCI_TD* td)
{
    console_print("td=0x%08x\n", td);
    console_print(" link=0x%08x\n", td->link);
    console_print(" alt_link=0x%08x\n", td->alt_link);
    console_print(" token=0x%08x\n", td->token);
    console_print(" buffer=0x%08x\n", td->buffer[0]);
}

// ------------------------------------------------------------------------------------------------
static void ehci_print_qh(EHCI_QH* qh)
{
    console_print("qh=0x%08x\n", qh);
    console_print(" qhlp=0x%08x\n", qh->qhlp);
    console_print(" ch=0x%08x\n", qh->ch);
    console_print(" caps=0x%08x\n", qh->caps);
    console_print(" cur_link=0x%08x\n", qh->cur_link);
    console_print(" next_link=0x%08x\n", qh->next_link);
    console_print(" alt_link=0x%08x\n", qh->alt_link);
    console_print(" token=0x%08x\n", qh->token);
    console_print(" buffer=0x%08x\n", qh->buffer[0]);
    console_print(" qh_prev=0x%08x\n", qh->qh_prev);
    console_print(" qh_next=0x%08x\n", qh->qh_next);
}
#endif

// ------------------------------------------------------------------------------------------------
static EHCI_TD* ehci_alloc_td(EHCI_Controller* hc)
{
    // TODO - better memory management
    EHCI_TD* end = hc->td_pool + MAX_TD;
    for (EHCI_TD* td = hc->td_pool; td != end; ++td)
    {
        if (!td->active)
        {
            //console_print("ehci_alloc_td 0x%08x\n", td);
            td->active = 1;
            return td;
        }
    }

    console_print("ehci_alloc_td failed\n");
    return 0;
}

// ------------------------------------------------------------------------------------------------
static EHCI_QH* ehci_alloc_qh(EHCI_Controller* hc)
{
    // TODO - better memory management
    EHCI_QH* end = hc->qh_pool + MAX_QH;
    for (EHCI_QH* qh = hc->qh_pool; qh != end; ++qh)
    {
        if (!qh->active)
        {
            //console_print("ehci_alloc_qh 0x%08x\n", qh);
            qh->active = 1;
            return qh;
        }
    }

    console_print("ehci_alloc_qh failed\n");
    return 0;
}

// ------------------------------------------------------------------------------------------------
static void ehci_free_td(EHCI_TD* td)
{
    //console_print("ehci_free_td 0x%08x\n", td);
    td->active = 0;
}

// ------------------------------------------------------------------------------------------------
static void ehci_free_qh(EHCI_QH* qh)
{
    //console_print("ehci_free_qh 0x%08x\n", qh);
    qh->active = 0;
}

// ------------------------------------------------------------------------------------------------
static void ehci_insert_async_qh(EHCI_QH* list, EHCI_QH* qh)
{
    EHCI_QH* end = link_data(list->qh_link.prev, EHCI_QH, qh_link);

    qh->qhlp = (u32)(uintptr_t)list | PTR_QH;
    end->qhlp = (u32)(uintptr_t)qh | PTR_QH;

    link_before(&list->qh_link, &qh->qh_link);
}

// ------------------------------------------------------------------------------------------------
static void ehci_insert_periodic_qh(EHCI_QH* list, EHCI_QH* qh)
{
    EHCI_QH* end = link_data(list->qh_link.prev, EHCI_QH, qh_link);

    qh->qhlp = PTR_TERMINATE;
    end->qhlp = (u32)(uintptr_t)qh | PTR_QH;

    link_before(&list->qh_link, &qh->qh_link);
}

// ------------------------------------------------------------------------------------------------
static void ehci_remove_qh(EHCI_QH* qh)
{
    EHCI_QH* prev = link_data(qh->qh_link.prev, EHCI_QH, qh_link);

    prev->qhlp = qh->qhlp;
    link_remove(&qh->qh_link);
}

// ------------------------------------------------------------------------------------------------
static void ehci_port_set(volatile u32* port_reg, u32 data)
{
    u32 status = *port_reg;
    status |= data;
    status &= ~PORT_RWC;
    *port_reg = status;
}

// ------------------------------------------------------------------------------------------------
static void ehci_port_clr(volatile u32* port_reg, u32 data)
{
    u32 status = *port_reg;
    status &= ~PORT_RWC;
    status &= ~data;
    status |= PORT_RWC & data;
    *port_reg = status;
}

// ------------------------------------------------------------------------------------------------
static void ehci_td_init(EHCI_TD* td, EHCI_TD* prev,
                         uint toggle, uint packet_type,
                         uint len, const void* data)
{
    if (prev)
    {
        prev->link = (u32)(uintptr_t)td;
        prev->td_next = (u32)(uintptr_t)td;
    }

    td->link = PTR_TERMINATE;
    td->alt_link = PTR_TERMINATE;
    td->td_next = 0;

    td->token =
        (toggle << TD_TOK_D_SHIFT) |
        (len << TD_TOK_LEN_SHIFT) |
        (3 << TD_TOK_CERR_SHIFT) |
        (packet_type << TD_TOK_PID_SHIFT) |
        TD_TOK_ACTIVE;

    // Data buffer (not necessarily page aligned)
    uintptr_t p = (uintptr_t)data;
    td->buffer[0] = (u32)p;
    td->ext_buffer[0] = (u32)(p >> 32);
    p &= ~0xfff;

    // Remaining pages of buffer memory.
    for (uint i = 1; i < 4; ++i)
    {
        p += 0x1000;
        td->buffer[i] = (u32)(p);
        td->ext_buffer[i] = (u32)(p >> 32);
    }
}

// ------------------------------------------------------------------------------------------------
static void ehci_qh_init(EHCI_QH* qh, USB_Transfer* t, EHCI_TD* td, USB_Device* parent, bool interrupt, uint speed, uint addr, uint endp, uint max_size)
{
    qh->transfer = t;

    uint ch =
        (max_size << QH_CH_MPL_SHIFT) |
        QH_CH_DTC |
        (speed << QH_CH_EPS_SHIFT) |
        (endp << QH_CH_ENDP_SHIFT) |
        addr;
    uint caps =
        (1 << QH_CAP_MULT_SHIFT);

    if (!interrupt)
    {
        ch |= 5 << QH_CH_NAK_RL_SHIFT;
    }

    if (speed != USB_HIGH_SPEED && parent)
    {
        if (interrupt)
        {
            // split completion mask - complete on frames 2, 3, or 4
            caps |= (0x1c << QH_CAP_SPLIT_C_SHIFT);
        }
        else
        {
            ch |= QH_CH_CONTROL;
        }

        caps |=
            (parent->port << QH_CAP_PORT_SHIFT) |
            (parent->addr << QH_CAP_HUB_ADDR_SHIFT);
    }

    if (interrupt)
    {
        // interrupt schedule mask - start on frame 0
        caps |= (0x01 << QH_CAP_INT_SCHED_SHIFT);
    }

    qh->ch = ch;
    qh->caps = caps;

    qh->td_head = (u32)(uintptr_t)td;
    qh->next_link = (u32)(uintptr_t)td;
    qh->token = 0;
}

// ------------------------------------------------------------------------------------------------
static void ehci_qh_process(EHCI_Controller* hc, EHCI_QH* qh)
{
    USB_Transfer* t = qh->transfer;

    if (qh->token & TD_TOK_HALTED)
    {
        t->success = false;
        t->complete = true;
    }
    else if (qh->next_link & PTR_TERMINATE)
    {
        if (~qh->token & TD_TOK_ACTIVE)
        {
            if (qh->token & TD_TOK_DATABUFFER)
            {
                console_print(" Data Buffer Error\n");
            }
            if (qh->token & TD_TOK_BABBLE)
            {
                console_print(" Babble Detected\n");
            }
            if (qh->token & TD_TOK_XACT)
            {
                console_print(" Transaction Error\n");
            }
            if (qh->token & TD_TOK_MMF)
            {
                console_print(" Missed Micro-Frame\n");
            }

            t->success = true;
            t->complete = true;
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
        ehci_remove_qh(qh);

        // Free transfer descriptors
        EHCI_TD* td = (EHCI_TD*)(uintptr_t)qh->td_head;
        while (td)
        {
            EHCI_TD* next = (EHCI_TD*)(uintptr_t)td->td_next;
            ehci_free_td(td);
            td = next;
        }

        // Free queue head
        ehci_free_qh(qh);
    }
}

// ------------------------------------------------------------------------------------------------
static void ehci_qh_wait(EHCI_Controller* hc, EHCI_QH* qh)
{
    USB_Transfer* t = qh->transfer;

    while (!t->complete)
    {
        ehci_qh_process(hc, qh);
    }
}

// ------------------------------------------------------------------------------------------------
static uint ehci_reset_port(EHCI_Controller* hc, uint port)
{
    volatile u32* reg = &hc->op_regs->ports[port];

    // Reset the port
    ehci_port_set(reg, PORT_RESET);
    pit_wait(50);
    ehci_port_clr(reg, PORT_RESET);

    // Wait 100ms for port to enable (TODO - what is appropriate length of time?)
    uint status = 0;
    for (uint i = 0; i < 10; ++i)
    {
        // Delay
        pit_wait(10);

        // Get current status
        status = *reg;

        // Check if device is attached to port
        if (~status & PORT_CONNECTION)
        {
            break;
        }

        // Acknowledge change in status
        if (status & (PORT_ENABLE_CHANGE | PORT_CONNECTION_CHANGE))
        {
            ehci_port_clr(reg, PORT_ENABLE_CHANGE | PORT_CONNECTION_CHANGE);
            continue;
        }

        // Check if device is enabled
        if (status & PORT_ENABLE)
        {
            break;
        }
    }

    return status;
}

// ------------------------------------------------------------------------------------------------
static void ehci_dev_control(USB_Device* dev, USB_Transfer* t)
{
    EHCI_Controller* hc = (EHCI_Controller*)dev->hc;
    USB_DevReq* req = t->req;

    // Determine transfer properties
    uint speed = dev->speed;
    uint addr = dev->addr;
    uint max_size = dev->max_packet_size;
    uint type = req->type;
    uint len = req->len;

    // Create queue of transfer descriptors
    EHCI_TD* td = ehci_alloc_td(hc);
    if (!td)
    {
        return;
    }

    EHCI_TD* head = td;
    EHCI_TD* prev = 0;

    // Setup packet
    uint toggle = 0;
    uint packet_type = USB_PACKET_SETUP;
    uint packet_size = sizeof(USB_DevReq);
    ehci_td_init(td, prev, toggle, packet_type, packet_size, req);
    prev = td;

    // Data in/out packets
    packet_type = type & RT_DEV_TO_HOST ? USB_PACKET_IN : USB_PACKET_OUT;

    u8* it = (u8*)t->data;
    u8* end = it + len;
    while (it < end)
    {
        td = ehci_alloc_td(hc);
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

        ehci_td_init(td, prev, toggle, packet_type, packet_size, it);

        it += packet_size;
        prev = td;
    }

    // Status packet
    td = ehci_alloc_td(hc);
    if (!td)
    {
        return;
    }

    toggle = 1;
    packet_type = type & RT_DEV_TO_HOST ? USB_PACKET_OUT : USB_PACKET_IN;
    ehci_td_init(td, prev, toggle, packet_type, 0, 0);

    // Initialize queue head
    EHCI_QH* qh = ehci_alloc_qh(hc);
    ehci_qh_init(qh, t, head, dev->parent, false, speed, addr, 0, max_size);

    // Wait until queue has been processed
    ehci_insert_async_qh(hc->async_qh, qh);
    ehci_qh_wait(hc, qh);
}

// ------------------------------------------------------------------------------------------------
static void ehci_dev_intr(USB_Device* dev, USB_Transfer* t)
{
    EHCI_Controller* hc = (EHCI_Controller*)dev->hc;

    // Determine transfer properties
    uint speed = dev->speed;
    uint addr = dev->addr;
    uint max_size = dev->max_packet_size;
    uint endp = dev->endp.desc.addr & 0xf;

    // Create queue of transfer descriptors
    EHCI_TD* td = ehci_alloc_td(hc);
    if (!td)
    {
        t->success = false;
        t->complete = true;
        return;
    }

    EHCI_TD* head = td;
    EHCI_TD* prev = 0;

    // Data in/out packets
    uint toggle = dev->endp.toggle;
    uint packet_type = USB_PACKET_IN;
    uint packet_size = t->len;

    ehci_td_init(td, prev, toggle, packet_type, packet_size, t->data);

    // Initialize queue head
    EHCI_QH* qh = ehci_alloc_qh(hc);
    ehci_qh_init(qh, t, head, dev->parent, true, speed, addr, endp, max_size);

    // Schedule queue
    ehci_insert_periodic_qh(hc->periodic_qh, qh);
}

// ------------------------------------------------------------------------------------------------
static void ehci_probe(EHCI_Controller* hc)
{
    // Port setup
    uint port_count = hc->cap_regs->hcs_params & HCSPARAMS_N_PORTS_MASK;
    for (uint port = 0; port < port_count; ++port)
    {
        // Reset port
        uint status = ehci_reset_port(hc, port);

        if (status & PORT_ENABLE)
        {
            uint speed = USB_HIGH_SPEED;

            USB_Device* dev = usb_dev_create();
            if (dev)
            {
                dev->parent = 0;
                dev->hc = hc;
                dev->port = port;
                dev->speed = speed;
                dev->max_packet_size = 8;

                dev->hc_control = ehci_dev_control;
                dev->hc_intr = ehci_dev_intr;

                if (!usb_dev_init(dev))
                {
                    // TODO - cleanup
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
static void ehci_controller_poll_list(EHCI_Controller* hc, Link* list)
{
    Link* it = list->next;
    Link* end = list;

    while (it != end)
    {
        EHCI_QH* qh = link_data(it, EHCI_QH, qh_link);
        Link* next = it->next;

        if (qh->transfer)
        {
            ehci_qh_process(hc, qh);
        }

        it = next;
    }
}

// ------------------------------------------------------------------------------------------------
static void ehci_controller_poll(USB_Controller* controller)
{
    EHCI_Controller* hc = (EHCI_Controller*)controller->hc;

    ehci_controller_poll_list(hc, &hc->async_qh->qh_link);
    ehci_controller_poll_list(hc, &hc->periodic_qh->qh_link);
}

// ------------------------------------------------------------------------------------------------
void ehci_init(uint id, PCI_DeviceInfo* info)
{
    if (!(((info->class_code << 8) | info->subclass) == PCI_SERIAL_USB &&
        info->prog_intf == PCI_SERIAL_USB_EHCI))
    {
        return;
    }

    if (sizeof(EHCI_QH) != 128)
    {
        console_print("Unexpected EHCI_QH size: %d\n", sizeof(EHCI_QH));
        return;
    }

    console_print ("Initializing EHCI\n");

    // Base I/O Address
    PCI_Bar bar;
    pci_get_bar(&bar, id, 0);
    if (bar.flags & PCI_BAR_IO)
    {
        // Only Memory Mapped I/O supported
        return;
    }

    // Controller initialization
    EHCI_Controller* hc = vm_alloc(sizeof(EHCI_Controller));
    hc->cap_regs = (EHCI_Cap_Regs*)(uintptr_t)bar.u.address;
    hc->op_regs = (EHCI_Op_Regs*)(uintptr_t)(bar.u.address + hc->cap_regs->cap_length);
    hc->frame_list = (u32*)vm_alloc(1024 * sizeof(u32));
    hc->qh_pool = (EHCI_QH*)vm_alloc(sizeof(EHCI_QH) * MAX_QH);
    hc->td_pool = (EHCI_TD*)vm_alloc(sizeof(EHCI_TD) * MAX_TD);

    memset(hc->qh_pool, 0, sizeof(EHCI_QH) * MAX_QH);
    memset(hc->td_pool, 0, sizeof(EHCI_TD) * MAX_TD);

    // Asynchronous queue setup
    EHCI_QH* qh = ehci_alloc_qh(hc);
    qh->qhlp = (u32)(uintptr_t)qh | PTR_QH;
    qh->ch = QH_CH_H;
    qh->caps = 0;
    qh->cur_link = 0;
    qh->next_link = PTR_TERMINATE;
    qh->alt_link = 0;
    qh->token = 0;
    for (uint i = 0; i < 5; ++i)
    {
        qh->buffer[i] = 0;
        qh->ext_buffer[i] = 0;
    }
    qh->transfer = 0;
    qh->qh_link.prev = &qh->qh_link;
    qh->qh_link.next = &qh->qh_link;

    hc->async_qh = qh;

    // Periodic list queue setup
    qh = ehci_alloc_qh(hc);
    qh->qhlp = PTR_TERMINATE;
    qh->ch = 0;
    qh->caps = 0;
    qh->cur_link = 0;
    qh->next_link = PTR_TERMINATE;
    qh->alt_link = 0;
    qh->token = 0;
    for (uint i = 0; i < 5; ++i)
    {
        qh->buffer[i] = 0;
        qh->ext_buffer[i] = 0;
    }
    qh->transfer = 0;
    qh->qh_link.prev = &qh->qh_link;
    qh->qh_link.next = &qh->qh_link;

    hc->periodic_qh = qh;
    for (uint i = 0; i < 1024; ++i)
    {
        hc->frame_list[i] = PTR_QH | (u32)(uintptr_t)qh;
    }

    // Check extended capabilities
    uint eecp = (hc->cap_regs->hcc_params & HCCPARAMS_EECP_MASK) >> HCCPARAMS_EECP_SHIFT;
    if (eecp >= 0x40)
    {
        // Disable BIOS legacy support
        uint legsup = pci_in32(id, eecp + USBLEGSUP);

        if (legsup & USBLEGSUP_HC_BIOS)
        {
            pci_out32(id, eecp + USBLEGSUP, legsup | USBLEGSUP_HC_OS);
            for (;;)
            {
                legsup = pci_in32(id, eecp + USBLEGSUP);
                if (~legsup & USBLEGSUP_HC_BIOS && legsup & USBLEGSUP_HC_OS)
                {
                    break;
                }
            }
        }
    }

    // Disable interrupts
    hc->op_regs->usb_intr = 0;

    // Setup frame list
    hc->op_regs->frame_index = 0;
    hc->op_regs->periodic_list_base = (u32)(uintptr_t)hc->frame_list;
    hc->op_regs->async_list_addr = (u32)(uintptr_t)hc->async_qh;
    hc->op_regs->ctrl_ds_segment = 0;

    // Clear status
    hc->op_regs->usb_sts = 0x3f;

    // Enable controller
    hc->op_regs->usb_cmd = (8 << CMD_ITC_SHIFT) | CMD_PSE | CMD_ASE | CMD_RS;
    while (hc->op_regs->usb_sts & STS_HCHALTED) // TODO - remove after dynamic port detection
        ;

    // Configure all devices to be managed by the EHCI
    hc->op_regs->config_flag = 1;
    pit_wait(5);    // TODO - remove after dynamic port detection

    // Probe devices
    ehci_probe(hc);

    // Register controller
    USB_Controller* controller = (USB_Controller*)vm_alloc(sizeof(USB_Controller));
    controller->next = g_usb_controller_list;
    controller->hc = hc;
    controller->poll = ehci_controller_poll;

    g_usb_controller_list = controller;
}
