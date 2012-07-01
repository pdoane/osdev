// ------------------------------------------------------------------------------------------------
// net/eth_8254x.c
// ------------------------------------------------------------------------------------------------

#include "net/eth_8254x.h"
#include "net/ipv4.h"
#include "net/eth.h"
#include "net/net.h"
#include "console/console.h"
#include "cpu/io.h"
#include "mem/vm.h"
#include "pci/driver.h"
#include "stdlib/string.h"

#define RX_DESC_COUNT                   32
#define TX_DESC_COUNT                   8

#define PACKET_SIZE                     2048

// ------------------------------------------------------------------------------------------------
// Receive Descriptor
typedef struct RX_Desc
{
    volatile u64 addr;
    volatile u16 len;
    volatile u16 checksum;
    volatile u8 status;
    volatile u8 errors;
    volatile u16 special;
} PACKED RX_Desc;

// ------------------------------------------------------------------------------------------------
// Receive Status

#define RSTA_DD                         (1 << 0)    // Descriptor Done
#define RSTA_EOP                        (1 << 1)    // End of Packet
#define RSTA_IXSM                       (1 << 2)    // Ignore Checksum Indication
#define RSTA_VP                         (1 << 3)    // Packet is 802.1Q
#define RSTA_TCPCS                      (1 << 5)    // TCP Checksum Calculated on Packet
#define RSTA_IPCS                       (1 << 6)    // IP Checksum Calculated on Packet
#define RSTA_PIF                        (1 << 7)    // Passed in-exact filter

// ------------------------------------------------------------------------------------------------
// Transmit Descriptor
typedef struct TX_Desc
{
    volatile u64 addr;
    volatile u16 len;
    volatile u8 cso;
    volatile u8 cmd;
    volatile u8 status;
    volatile u8 css;
    volatile u16 special;
} PACKED TX_Desc;

// ------------------------------------------------------------------------------------------------
// Transmit Command

#define CMD_EOP                         (1 << 0)    // End of Packet
#define CMD_IFCS                        (1 << 1)    // Insert FCS
#define CMD_IC                          (1 << 2)    // Insert Checksum
#define CMD_RS                          (1 << 3)    // Report Status
#define CMD_RPS                         (1 << 4)    // Report Packet Sent
#define CMD_VLE                         (1 << 6)    // VLAN Packet Enable
#define CMD_IDE                         (1 << 7)    // Interrupt Delay Enable

// ------------------------------------------------------------------------------------------------
// Transmit Status

#define TSTA_DD                         (1 << 0)    // Descriptor Done
#define TSTA_EC                         (1 << 1)    // Excess Collisions
#define TSTA_LC                         (1 << 2)    // Late Collision
#define LSTA_TU                         (1 << 3)    // Transmit Underrun

// ------------------------------------------------------------------------------------------------
// Device State
typedef struct Device
{
    u8* mmio_addr;
    uint rx_read;
    uint tx_write;
    RX_Desc* rx_descs;
    TX_Desc* tx_descs;
} Device;

static Device dev;

// ------------------------------------------------------------------------------------------------
// Registers

#define REG_CTRL                        0x0000      // Device Control
#define REG_EERD                        0x0014      // EEPROM Read
#define REG_ICR                         0x00c0      // Interrupt Cause Read
#define REG_IMS                         0x00d0      // Interrupt Mask Set/Read
#define REG_RCTL                        0x0100      // Receive Control
#define REG_TCTL                        0x0400      // Transmit Control
#define REG_RDBAL                       0x2800      // Receive Descriptor Base Low
#define REG_RDBAH                       0x2804      // Receive Descriptor Base High
#define REG_RDLEN                       0x2808      // Receive Descriptor Length
#define REG_RDH                         0x2810      // Receive Descriptor Head
#define REG_RDT                         0x2818      // Receive Descriptor Tail
#define REG_TDBAL                       0x3800      // Transmit Descriptor Base Low
#define REG_TDBAH                       0x3804      // Transmit Descriptor Base High
#define REG_TDLEN                       0x3808      // Transmit Descriptor Length
#define REG_TDH                         0x3810      // Transmit Descriptor Head
#define REG_TDT                         0x3818      // Transmit Descriptor Tail
#define REG_MTA                         0x5200      // Multicast Table Array
#define REG_RAL                         0x5400      // Receive Address Low
#define REG_RAH                         0x5404      // Receive Address High

// ------------------------------------------------------------------------------------------------
// Control Register

#define CTRL_SLU                        (1 << 6)    // Set Link Up

// ------------------------------------------------------------------------------------------------
// EERD Register

#define EERD_START                      0x0001      // Start Read
#define EERD_DONE                       0x0010      // Read Done
#define EERD_ADDR_SHIFT                 8
#define EERD_DATA_SHIFT                 16

// ------------------------------------------------------------------------------------------------
// RCTL Register

#define RCTL_EN                         (1 << 1)    // Receiver Enable
#define RCTL_SBP                        (1 << 2)    // Store Bad Packets
#define RCTL_UPE                        (1 << 3)    // Unicast Promiscuous Enabled
#define RCTL_MPE                        (1 << 4)    // Multicast Promiscuous Enabled
#define RCTL_LPE                        (1 << 5)    // Long Packet Reception Enable
#define RCTL_LBM_NONE                   (0 << 6)    // No Loopback
#define RCTL_LBM_PHY                    (3 << 6)    // PHY or external SerDesc loopback
#define RTCL_RDMTS_HALF                 (0 << 8)    // Free Buffer Threshold is 1/2 of RDLEN
#define RTCL_RDMTS_QUARTER              (1 << 8)    // Free Buffer Threshold is 1/4 of RDLEN
#define RTCL_RDMTS_EIGHTH               (2 << 8)    // Free Buffer Threshold is 1/8 of RDLEN
#define RCTL_MO_36                      (0 << 12)   // Multicast Offset - bits 47:36
#define RCTL_MO_35                      (1 << 12)   // Multicast Offset - bits 46:35
#define RCTL_MO_34                      (2 << 12)   // Multicast Offset - bits 45:34
#define RCTL_MO_32                      (3 << 12)   // Multicast Offset - bits 43:32
#define RCTL_BAM                        (1 << 15)   // Broadcast Accept Mode
#define RCTL_VFE                        (1 << 18)   // VLAN Filter Enable
#define RCTL_CFIEN                      (1 << 19)   // Canonical Form Indicator Enable
#define RCTL_CFI                        (1 << 20)   // Canonical Form Indicator Bit Value
#define RCTL_DPF                        (1 << 22)   // Discard Pause Frames
#define RCTL_PMCF                       (1 << 23)   // Pass MAC Control Frames
#define RCTL_SECRC                      (1 << 26)   // Strip Ethernet CRC

// Buffer Sizes
#define RCTL_BSIZE_256                  (3 << 16)
#define RCTL_BSIZE_512                  (2 << 16)
#define RCTL_BSIZE_1024                 (1 << 16)
#define RCTL_BSIZE_2048                 (0 << 16)
#define RCTL_BSIZE_4096                 ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192                 ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384                ((1 << 16) | (1 << 25))

// ------------------------------------------------------------------------------------------------
// TCTL Register

#define TCTL_EN                         (1 << 1)    // Transmit Enable
#define TCTL_PSP                        (1 << 3)    // Pad Short Packets
#define TCTL_CT_SHIFT                   4           // Collision Threshold
#define TCTL_COLD_SHIFT                 12          // Collision Distance
#define TCTL_SWXOFF                     (1 << 22)   // Software XOFF Transmission
#define TCTL_RTLC                       (1 << 24)   // Re-transmit on Late Collision

// ------------------------------------------------------------------------------------------------
static u16 eeprom_read(u8* mmio_addr, u8 eeprom_addr)
{
    mmio_write32(mmio_addr + REG_EERD, EERD_START | (eeprom_addr << EERD_ADDR_SHIFT));

    u32 val;
    do
    {
        // TODO - add some kind of delay here
        val = mmio_read32(mmio_addr + REG_EERD);
    }
    while (~val & EERD_DONE);

    return val >> EERD_DATA_SHIFT;
}

// ------------------------------------------------------------------------------------------------
static void eth_8254x_poll(Net_Intf* intf)
{
    RX_Desc* desc = &dev.rx_descs[dev.rx_read];

    while (desc->status & RSTA_DD)
    {
        u8* pkt = (u8*)desc->addr;
        uint len = desc->len;

        if (desc->errors)
        {
            console_print("Packet Error: (0x%x)\n", desc->errors);
        }
        else
        {
            eth_rx(intf, pkt, len);
        }

        desc->status = 0;

        mmio_write32(dev.mmio_addr + REG_RDT, dev.rx_read);

        dev.rx_read = (dev.rx_read + 1) & (RX_DESC_COUNT - 1);
        desc = &dev.rx_descs[dev.rx_read];
    }
}

// ------------------------------------------------------------------------------------------------
static void eth_8254x_tx(u8* pkt, uint len)
{
    TX_Desc* desc = &dev.tx_descs[dev.tx_write];

    // Check for available tx descriptor
    if (desc->status & TSTA_DD)
    {
        // Free packet that was sent with this descriptor.
        // TODO - free packets earlier?

        if (desc->addr)
       {
            NetBuf* buf = (NetBuf*)((uintptr_t)desc->addr & ~0xfff);  // buffer starts page aligned
            net_free_packet(buf);
        }
    }
    else
    {
        // TODO - report overflow
        console_print("Net overflow\n");
        return;
    }

    // Write new tx descriptor
    desc->addr = (u64)pkt;
    desc->len = len;
    desc->cmd = CMD_EOP | CMD_IFCS | CMD_RS;

    dev.tx_write = (dev.tx_write + 1) & (TX_DESC_COUNT - 1);
    mmio_write32(dev.mmio_addr + REG_TDT, dev.tx_write);
}

// ------------------------------------------------------------------------------------------------
void eth_8254x_init(uint id, PCI_DeviceInfo* info)
{
    // Check device supported.
    if (info->vendor_id != 0x8086)
    {
        return;
    }

    if (!(info->device_id == 0x100e || info->device_id == 0x1503))
    {
        return;
    }

    console_print("Initializing Intel Gigabit Ethernet\n");

    // Base I/O Address
    u32 bar0 = pci_in32(id, PCI_CONFIG_BAR0);
    if (bar0 & 0x1)
    {
        // Only Memory Mapped I/O supported
        return;
    }
    if (bar0 & 0x4)
    {
        // TODO - support 64-bit pointer
        return;
    }

    bar0 &= ~0xf;    // clear low 4 bits

    u8* mmio_addr = (u8*)(uintptr_t)bar0;
    dev.mmio_addr = mmio_addr;

    // IRQ
    //u8 irq = pci_in8(id, PCI_CONFIG_INTERRUPT_LINE);

    // MAC address
    Eth_Addr local_addr;
    u32 ral = mmio_read32(mmio_addr + REG_RAL);   // Try Receive Address Register first
    if (ral)
    {
        u32 rah = mmio_read32(mmio_addr + REG_RAH);

        local_addr.n[0] = (u8)(ral);
        local_addr.n[1] = (u8)(ral >> 8);
        local_addr.n[2] = (u8)(ral >> 16);
        local_addr.n[3] = (u8)(ral >> 24);
        local_addr.n[4] = (u8)(rah);
        local_addr.n[5] = (u8)(rah >> 8);
    }
    else
    {
        // Read MAC address from EEPROM registers
        u16 mac01 = eeprom_read(mmio_addr, 0);
        u16 mac23 = eeprom_read(mmio_addr, 1);
        u16 mac45 = eeprom_read(mmio_addr, 2);

        local_addr.n[0] = (u8)(mac01);
        local_addr.n[1] = (u8)(mac01 >> 8);
        local_addr.n[2] = (u8)(mac23);
        local_addr.n[3] = (u8)(mac23 >> 8);
        local_addr.n[4] = (u8)(mac45);
        local_addr.n[5] = (u8)(mac45 >> 8);
    }

    char mac_str[18];
    eth_addr_to_str(mac_str, sizeof(mac_str), &local_addr);

    console_print("MAC = %s\n", mac_str);

    // Set Link Up
    mmio_write32(mmio_addr + REG_CTRL, mmio_read32(mmio_addr + REG_CTRL) | CTRL_SLU);

    // Clear Multicast Table Array
    for (int i = 0; i < 128; ++i)
    {
        mmio_write32(mmio_addr + REG_MTA + (i * 4), 0);
    }

    // Enable interrupts (TODO - only enable specific types as some of these are reserved bits)
    //mmio_write32(mmio_addr + REG_IMS, 0x1ffff);

    // Clear all interrupts
    mmio_read32(mmio_addr + REG_ICR);

    // Allocate memory
    u8* rx_packet = vm_alloc(RX_DESC_COUNT * PACKET_SIZE);
    RX_Desc* rx_descs = vm_alloc(RX_DESC_COUNT * sizeof(RX_Desc));
    TX_Desc* tx_descs = vm_alloc(TX_DESC_COUNT * sizeof(TX_Desc));

    dev.rx_descs = rx_descs;
    dev.tx_descs = tx_descs;

    // Receive Setup
    RX_Desc* rx_desc = rx_descs;
    RX_Desc* rx_end = rx_desc + RX_DESC_COUNT;
    for (; rx_desc != rx_end; ++rx_desc, rx_packet += PACKET_SIZE)
    {
        rx_desc->addr = (u64)rx_packet;
        rx_desc->status = 0;
    }

    dev.rx_read = 0;

    mmio_write32(mmio_addr + REG_RDBAL, (uintptr_t)rx_descs);
    mmio_write32(mmio_addr + REG_RDBAH, (uintptr_t)rx_descs >> 32);
    mmio_write32(mmio_addr + REG_RDLEN, RX_DESC_COUNT * 16);
    mmio_write32(mmio_addr + REG_RDH, 0);
    mmio_write32(mmio_addr + REG_RDT, RX_DESC_COUNT - 1);
    mmio_write32(mmio_addr + REG_RCTL,
          RCTL_EN
        | RCTL_SBP
        | RCTL_UPE
        | RCTL_MPE
        | RCTL_LBM_NONE
        | RTCL_RDMTS_HALF
        | RCTL_BAM
        | RCTL_SECRC
        | RCTL_BSIZE_2048
        );

    // Transmit Setup
    TX_Desc* tx_desc = tx_descs;
    TX_Desc* tx_end = tx_desc + TX_DESC_COUNT;
    memset(tx_desc, 0, TX_DESC_COUNT * 16);

    for (; tx_desc != tx_end; ++tx_desc)
    {
        tx_desc->status = TSTA_DD;      // mark descriptor as 'complete'
    }

    dev.tx_write = 0;

    mmio_write32(mmio_addr + REG_TDBAL, (uintptr_t)tx_descs);
    mmio_write32(mmio_addr + REG_TDBAH, (uintptr_t)tx_descs >> 32);
    mmio_write32(mmio_addr + REG_TDLEN, TX_DESC_COUNT * 16);
    mmio_write32(mmio_addr + REG_TDH, 0);
    mmio_write32(mmio_addr + REG_TDT, 0);
    mmio_write32(mmio_addr + REG_TCTL,
          TCTL_EN
        | TCTL_PSP
        | (15 << TCTL_CT_SHIFT)
        | (64 << TCTL_COLD_SHIFT)
        | TCTL_RTLC
        );

    // Create net interface
    Net_Intf* intf = net_intf_create();
    intf->eth_addr = local_addr;
    intf->ip_addr = null_ipv4_addr;
    intf->name = "eth";
    intf->poll = eth_8254x_poll;
    intf->tx = eth_tx_intf;
    intf->dev_tx = eth_8254x_tx;

    net_intf_add(intf);
}
