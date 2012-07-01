// ------------------------------------------------------------------------------------------------
// pci/driver.c
// ------------------------------------------------------------------------------------------------

#include "pci/driver.h"
#include "cpu/io.h"

#include "gfx/gfx.h"
#include "net/eth_8254x.h"
#include "usb/ehci.h"
#include "usb/uhci.h"

// ------------------------------------------------------------------------------------------------
PCI_Driver pci_driver_table[] =
{
    { eth_8254x_init },
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