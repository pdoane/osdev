// ------------------------------------------------------------------------------------------------
// pci/driver.c
// ------------------------------------------------------------------------------------------------

#include "pci/driver.h"
#include "cpu/io.h"

#include "gfx/gfx.h"
#include "net/intel.h"
#include "usb/ehci.h"
#include "usb/uhci.h"

// ------------------------------------------------------------------------------------------------
const PCI_Driver pci_driver_table[] =
{
    { eth_intel_init },
    { uhci_init },
    { ehci_init },
    { gfx_init },
    { 0 },
};

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
void pci_out8(uint id, uint reg, u8 data)
{
    u32 address = 0x80000000 | id | (reg & 0xfc);
    out32(PCI_CONFIG_ADDR, address);
    out8(PCI_CONFIG_DATA + (reg & 0x03), data);
}

// ------------------------------------------------------------------------------------------------
void pci_out16(uint id, uint reg, u16 data)
{
    u32 address = 0x80000000 | id | (reg & 0xfc);
    out32(PCI_CONFIG_ADDR, address);
    out16(PCI_CONFIG_DATA + (reg & 0x02), data);
}

// ------------------------------------------------------------------------------------------------
void pci_out32(uint id, uint reg, u32 data)
{
    u32 address = 0x80000000 | id | (reg & 0xfc);
    out32(PCI_CONFIG_ADDR, address);
    out32(PCI_CONFIG_DATA, data);
}

// ------------------------------------------------------------------------------------------------
static void pci_read_bar(uint id, uint index, u32* address, u32* mask)
{
    uint reg = PCI_CONFIG_BAR0 + index * sizeof(u32);

    // Get address
    *address = pci_in32(id, reg);

    // Find out size of the bar
    pci_out32(id, reg, 0xffffffff);
    *mask = pci_in32(id, reg);

    // Restore adddress
    pci_out32(id, reg, *address);
}

// ------------------------------------------------------------------------------------------------
void pci_get_bar(PCI_Bar* bar, uint id, uint index)
{
    // Read pci bar register
    u32 address_low;
    u32 mask_low;
    pci_read_bar(id, index, &address_low, &mask_low);

    if (address_low & PCI_BAR_64)
    {
        // 64-bit mmio
        u32 address_hi;
        u32 mask_hi;
        pci_read_bar(id, index + 1, &address_hi, &mask_hi);

        bar->u.address = (void*)(((uintptr_t)address_hi << 32) | (address_low & ~0xf));
        bar->size = ~(((u64)mask_hi << 32) | (mask_low & ~0xf)) + 1;
        bar->flags = address_low & 0xf;
    }
    else if (address_low & PCI_BAR_IO)
    {
        // i/o register
        bar->u.port = (u16)(address_low & ~0x3);
        bar->size = (u16)(~(mask_low & ~0x3) + 1);
        bar->flags = address_low & 0x3;
    }
    else
    {
        // 32-bit mmio
        bar->u.address = (void*)(uintptr_t)(address_low & ~0xf);
        bar->size = ~(mask_low & ~0xf) + 1;
        bar->flags = address_low & 0xf;
    }
}
