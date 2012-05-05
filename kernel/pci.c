// ------------------------------------------------------------------------------------------------
// pci.c
// ------------------------------------------------------------------------------------------------

#include "pci.h"
#include "console.h"
#include "io.h"
#include "pci_classify.h"

// ------------------------------------------------------------------------------------------------
#define PCI_MAKE_ID(bus, dev, func)     ((bus) << 16) | ((dev) << 11) | ((func) << 8)

// I/O Ports
#define PCI_CONFIG_ADDR                 0xcf8
#define PCI_CONFIG_DATA                 0xcfC

// Header Type
#define PCI_TYPE_MULTIFUNC              0x80
#define PCI_TYPE_GENERIC                0x00
#define PCI_TYPE_PCI_BRIDGE             0x01
#define PCI_TYPE_CARDBUS_BRIDGE         0x02

// PCI Configuration Registers
#define PCI_CONFIG_VENDOR_ID            0x00
#define PCI_CONFIG_DEVICE_ID            0x02
#define PCI_CONFIG_COMMAND              0x04
#define PCI_CONFIG_STATUS               0x06
#define PCI_CONFIG_REVISION_ID          0x08
#define PCI_CONFIG_PROG_INTF            0x09
#define PCI_CONFIG_SUBCLASS             0x0a
#define PCI_CONFIG_CLASS_CODE           0x0b
#define PCI_CONFIG_CACHELINE_SIZE       0x0c
#define PCI_CONFIG_LATENCY              0x0d
#define PCI_CONFIG_HEADER_TYPE          0x0e
#define PCI_CONFIG_BIST                 0x0f

// Type 0x00 (Generic) Configuration Registers
#define PCI_CONFIG_BASE_ADDR0           0x10
#define PCI_CONFIG_BASE_ADDR1           0x14
#define PCI_CONFIG_BASE_ADDR2           0x18
#define PCI_CONFIG_BASE_ADDR3           0x1c
#define PCI_CONFIG_BASE_ADDR4           0x20
#define PCI_CONFIG_BASE_ADDR5           0x24
#define PCI_CONFIG_CARDBUS_CIS          0x28
#define PCI_CONFIG_SUBSYSTEM_VENDOR_ID  0x2c
#define PCI_CONFIG_SUBSYSTEM_DEVICE_ID  0x2e
#define PCI_CONFIG_EXPANSION_ROM        0x30
#define PCI_CONFIG_CAPABILITIES         0x34
#define PCI_CONFIG_INTERRUPT_LINE       0x3c
#define PCI_CONFIG_INTERRUPT_PIN        0x3d
#define PCI_CONFIG_MIN_GRANT            0x3e
#define PCI_CONFIG_MAX_LATENCY          0x3f

// ------------------------------------------------------------------------------------------------
u8 pci_in8(uint id, uint reg)
{
    u32 addr = 0x80000000 | id | (reg & 0xfc);
    out32(PCI_CONFIG_ADDR, addr);
    return in8(PCI_CONFIG_DATA + (reg & 0x03));
}

// ------------------------------------------------------------------------------------------------
u16 pci_in16(uint id, uint reg)
{
    u32 addr = 0x80000000 | id | (reg & 0xfc);
    out32(PCI_CONFIG_ADDR, addr);
    return in16(PCI_CONFIG_DATA + (reg & 0x02));
}

// ------------------------------------------------------------------------------------------------
u32 pci_in32(uint id, uint reg)
{
    u32 addr = 0x80000000 | id | (reg & 0xfc);
    out32(PCI_CONFIG_ADDR, addr);
    return in32(PCI_CONFIG_DATA);
}

// ------------------------------------------------------------------------------------------------
static void pci_get_info(uint bus, uint dev, uint func)
{
    uint id = PCI_MAKE_ID(bus, dev, func);
    u16 vendor_id = pci_in16(id, PCI_CONFIG_VENDOR_ID);
    if (vendor_id == 0xffff)
    {
        return;
    }

    u16 device_id = pci_in16(id, PCI_CONFIG_DEVICE_ID);
    u8 prog_intf = pci_in8(id, PCI_CONFIG_PROG_INTF);
    u8 subclass = pci_in8(id, PCI_CONFIG_SUBCLASS);
    u8 class_code = pci_in8(id, PCI_CONFIG_CLASS_CODE);

    console_print("%02x:%02x:%d 0x%04x/0x%04x: %s\n",
        bus, dev, func,
        vendor_id, device_id,
        pci_class_name(class_code, subclass, prog_intf)
        );
}

// ------------------------------------------------------------------------------------------------
void pci_init()
{
    console_print("Enumerating PCI Devices\n");
    for (uint bus = 0; bus < 256; ++bus)
    {
        for (uint dev = 0; dev < 32; ++dev)
        {
            uint base_id = PCI_MAKE_ID(bus, dev, 0);
            u8 header_type = pci_in8(base_id, PCI_CONFIG_HEADER_TYPE);
            uint func_count = header_type & PCI_TYPE_MULTIFUNC ? 8 : 1;

            for (uint func = 0; func < func_count; ++func)
            {
                pci_get_info(bus, dev, func);
            }
        }
    }
}
