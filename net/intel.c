// ------------------------------------------------------------------------------------------------
// net/intel.c
// ------------------------------------------------------------------------------------------------

#include "net/intel.h"
#include "net/buf.h"
#include "net/ipv4.h"
#include "net/eth.h"
#include "console/console.h"
#include "cpu/io.h"
#include "mem/vm.h"
#include "pci/driver.h"
#include "stdlib/string.h"
#include "time/pit.h"

#define RX_DESC_COUNT                   32
#define TX_DESC_COUNT                   8

#define PACKET_SIZE                     2048

// ------------------------------------------------------------------------------------------------
// Receive Descriptor
typedef struct RecvDesc
{
    volatile u64 addr;
    volatile u16 len;
    volatile u16 checksum;
    volatile u8 status;
    volatile u8 errors;
    volatile u16 special;
} PACKED RecvDesc;

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
typedef struct TransDesc
{
    volatile u64 addr;
    volatile u16 len;
    volatile u8 cso;
    volatile u8 cmd;
    volatile u8 status;
    volatile u8 css;
    volatile u16 special;
} PACKED TransDesc;

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

typedef struct EthIntelDevice
{
    u8 *mmioAddr;
    uint rxRead;
    uint txWrite;
    RecvDesc *rxDescs;
    TransDesc *txDescs;
    NetBuf *rxBufs[RX_DESC_COUNT];
    NetBuf *txBufs[TX_DESC_COUNT];
} EthIntelDevice;

static EthIntelDevice s_device;

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
static u16 EepromRead(u8 *mmioAddr, u8 eepromAddr)
{
    MmioWrite32(mmioAddr + REG_EERD, EERD_START | (eepromAddr << EERD_ADDR_SHIFT));

    u32 val;
    do
    {
        // TODO - add some kind of delay here
        val = MmioRead32(mmioAddr + REG_EERD);
    }
    while (~val & EERD_DONE);

    return val >> EERD_DATA_SHIFT;
}

// ------------------------------------------------------------------------------------------------
static void EthIntelPoll(NetIntf *intf)
{
    RecvDesc *desc = &s_device.rxDescs[s_device.rxRead];

    while (desc->status & RSTA_DD)
    {
        if (desc->errors)
        {
            ConsolePrint("Packet Error: (0x%x)\n", desc->errors);
        }
        else
        {
            NetBuf *buf = s_device.rxBufs[s_device.rxRead];
            buf->end = buf->start + desc->len;

            EthRecv(intf, buf);

            NetReleaseBuf(buf);
            buf = NetAllocBuf();
            desc->addr = (u64)(uintptr_t)buf->start;
        }

        desc->status = 0;

        MmioWrite32(s_device.mmioAddr + REG_RDT, s_device.rxRead);

        s_device.rxRead = (s_device.rxRead + 1) & (RX_DESC_COUNT - 1);
        desc = &s_device.rxDescs[s_device.rxRead];
    }
}

// ------------------------------------------------------------------------------------------------
static void EthIntelSend(NetBuf *buf)
{
    TransDesc *desc = &s_device.txDescs[s_device.txWrite];
    NetBuf *oldBuf = s_device.txBufs[s_device.txWrite];

    // Wait until packet is sent
    while (!(desc->status & 0xf))
    {
        PitWait(1);
    }

    // Free packet that was sent with this descriptor.
    // TODO - free packets earlier?

    if (oldBuf)
    {
        NetReleaseBuf(oldBuf);
    }

    // Write new tx descriptor
    desc->addr = (u64)(uintptr_t)buf->start;
    desc->len = buf->end - buf->start;
    desc->cmd = CMD_EOP | CMD_IFCS | CMD_RS;
    desc->status = 0;
    s_device.txBufs[s_device.txWrite] = buf;

    s_device.txWrite = (s_device.txWrite + 1) & (TX_DESC_COUNT - 1);
    MmioWrite32(s_device.mmioAddr + REG_TDT, s_device.txWrite);
}

// ------------------------------------------------------------------------------------------------
void EthIntelInit(uint id, PciDeviceInfo *info)
{
    // Check device supported.
    if (info->vendorId != 0x8086)
    {
        return;
    }

    if (!(info->deviceId == 0x100e || info->deviceId == 0x1503))
    {
        return;
    }

    ConsolePrint("Initializing Intel Gigabit Ethernet\n");

    // Base I/O Address
    PciBar bar;
    PciGetBar(&bar, id, 0);
    if (bar.flags & PCI_BAR_IO)
    {
        // Only Memory Mapped I/O supported
        return;
    }

    u8 *mmioAddr = (u8 *)bar.u.address;
    s_device.mmioAddr = mmioAddr;

    // IRQ
    //u8 irq = PciRead8(id, PCI_CONFIG_INTERRUPT_LINE);

    // MAC address
    EthAddr localAddr;
    u32 ral = MmioRead32(mmioAddr + REG_RAL);   // Try Receive Address Register first
    if (ral)
    {
        u32 rah = MmioRead32(mmioAddr + REG_RAH);

        localAddr.n[0] = (u8)(ral);
        localAddr.n[1] = (u8)(ral >> 8);
        localAddr.n[2] = (u8)(ral >> 16);
        localAddr.n[3] = (u8)(ral >> 24);
        localAddr.n[4] = (u8)(rah);
        localAddr.n[5] = (u8)(rah >> 8);
    }
    else
    {
        // Read MAC address from EEPROM registers
        u16 mac01 = EepromRead(mmioAddr, 0);
        u16 mac23 = EepromRead(mmioAddr, 1);
        u16 mac45 = EepromRead(mmioAddr, 2);

        localAddr.n[0] = (u8)(mac01);
        localAddr.n[1] = (u8)(mac01 >> 8);
        localAddr.n[2] = (u8)(mac23);
        localAddr.n[3] = (u8)(mac23 >> 8);
        localAddr.n[4] = (u8)(mac45);
        localAddr.n[5] = (u8)(mac45 >> 8);
    }

    char macStr[18];
    EthAddrToStr(macStr, sizeof(macStr), &localAddr);

    ConsolePrint("MAC = %s\n", macStr);

    // Set Link Up
    MmioWrite32(mmioAddr + REG_CTRL, MmioRead32(mmioAddr + REG_CTRL) | CTRL_SLU);

    // Clear Multicast Table Array
    for (int i = 0; i < 128; ++i)
    {
        MmioWrite32(mmioAddr + REG_MTA + (i * 4), 0);
    }

    // Enable interrupts (TODO - only enable specific types as some of these are reserved bits)
    //MmioWrite32(mmioAddr + REG_IMS, 0x1ffff);

    // Clear all interrupts
    MmioRead32(mmioAddr + REG_ICR);

    // Allocate memory
    RecvDesc *rxDescs = VMAlloc(RX_DESC_COUNT * sizeof(RecvDesc));
    TransDesc *txDescs = VMAlloc(TX_DESC_COUNT * sizeof(TransDesc));

    s_device.rxDescs = rxDescs;
    s_device.txDescs = txDescs;

    // Receive Setup
    for (uint i = 0; i < RX_DESC_COUNT; ++i)
    {
        NetBuf *buf = NetAllocBuf();

        s_device.rxBufs[i] = buf;

        RecvDesc *rxDesc = rxDescs + i;
        rxDesc->addr = (u64)(uintptr_t)buf->start;
        rxDesc->status = 0;
    }

    s_device.rxRead = 0;

    MmioWrite32(mmioAddr + REG_RDBAL, (uintptr_t)rxDescs);
    MmioWrite32(mmioAddr + REG_RDBAH, (uintptr_t)rxDescs >> 32);
    MmioWrite32(mmioAddr + REG_RDLEN, RX_DESC_COUNT * 16);
    MmioWrite32(mmioAddr + REG_RDH, 0);
    MmioWrite32(mmioAddr + REG_RDT, RX_DESC_COUNT - 1);
    MmioWrite32(mmioAddr + REG_RCTL,
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
    TransDesc *txDesc = txDescs;
    TransDesc *txEnd = txDesc + TX_DESC_COUNT;
    memset(txDesc, 0, TX_DESC_COUNT * 16);

    for (; txDesc != txEnd; ++txDesc)
    {
        txDesc->status = TSTA_DD;      // mark descriptor as 'complete'
    }

    s_device.txWrite = 0;

    MmioWrite32(mmioAddr + REG_TDBAL, (uintptr_t)txDescs);
    MmioWrite32(mmioAddr + REG_TDBAH, (uintptr_t)txDescs >> 32);
    MmioWrite32(mmioAddr + REG_TDLEN, TX_DESC_COUNT * 16);
    MmioWrite32(mmioAddr + REG_TDH, 0);
    MmioWrite32(mmioAddr + REG_TDT, 0);
    MmioWrite32(mmioAddr + REG_TCTL,
          TCTL_EN
        | TCTL_PSP
        | (15 << TCTL_CT_SHIFT)
        | (64 << TCTL_COLD_SHIFT)
        | TCTL_RTLC
        );

    // Create net interface
    NetIntf *intf = NetIntfCreate();
    intf->ethAddr = localAddr;
    intf->ipAddr = g_nullIpv4Addr;
    intf->name = "eth";
    intf->poll = EthIntelPoll;
    intf->send = EthSendIntf;
    intf->devSend = EthIntelSend;

    NetIntfAdd(intf);
}
