// ------------------------------------------------------------------------------------------------
// gfx/gfxpci.c
// ------------------------------------------------------------------------------------------------

#include "gfx/gfxpci.h"
#include "pci/driver.h"
#include "net/rlog.h"
#include "cpu/io.h"

void gfx_init_pci(GfxPCI *pPci)
{
    // Read PCI registers
    PCI_Bar bar;

    rlog_print("...Probing PCIe Config:\n");
    rlog_print("    PCI id:       0x%X\n",             pPci->id);

    // GTTMMADDR
    pci_get_bar(&bar, pPci->id, 0);
    pPci->mmio_bar = bar.u.address;
    pPci->gtt_addr = (u32*)((u8*)bar.u.address + (2 * 1024 * 1024));
    rlog_print("    GTTMMADR:     0x%llX (%llu MB)\n", bar.u.address, bar.size / (1024 * 1024));

    // GMADR
    pci_get_bar(&bar, pPci->id, 2);
    pPci->aperture_bar  = bar.u.address;
    pPci->aperture_size = bar.size;
    rlog_print("    GMADR:        0x%llX (%llu MB)\n", bar.u.address, bar.size / (1024 * 1024));

    // IOBASE
    pci_get_bar(&bar, pPci->id, 4);
    pPci->iobase = bar.u.port;
    rlog_print("    IOBASE:       0x%X (%u bytes)\n", bar.u.port, bar.size);
}

u32 gfx_read32(GfxPCI *pPci, uint reg)
{
    return mmio_read32((u8*)pPci->mmio_bar + reg);
}

u64 gfx_read64(GfxPCI *pPci, uint reg)
{
    return mmio_read64((u8*)pPci->mmio_bar + reg);
}

void gfx_write32(GfxPCI *pPci, uint reg, u32 value)
{
    mmio_write32((u8*)pPci->mmio_bar + reg, value);
}

void gfx_write64(GfxPCI *pPci, uint reg, u32 value)
{
    mmio_write64((u8*)pPci->mmio_bar + reg, value);
}