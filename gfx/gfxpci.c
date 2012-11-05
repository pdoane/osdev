// ------------------------------------------------------------------------------------------------
// gfx/gfxpci.c
// ------------------------------------------------------------------------------------------------

#include "gfx/gfxpci.h"
#include "pci/driver.h"
#include "net/rlog.h"
#include "cpu/io.h"

// ------------------------------------------------------------------------------------------------
void GfxInitPci(GfxPci *pci)
{
    // Read PCI registers
    PciBar bar;

    RlogPrint("...Probing PCIe Config:\n");
    RlogPrint("    PCI id:       0x%X\n",             pci->id);

    // GTTMMADDR
    PciGetBar(&bar, pci->id, 0);
    pci->mmioBar = bar.u.address;
    pci->gttAddr = (u32 *)((u8*)bar.u.address + 2 * MB);
    RlogPrint("    GTTMMADR:     0x%llX (%llu MB)\n", bar.u.address, bar.size / MB);

    // GMADR
    PciGetBar(&bar, pci->id, 2);
    pci->apertureBar  = bar.u.address;
    pci->apertureSize = bar.size;
    RlogPrint("    GMADR:        0x%llX (%llu MB)\n", bar.u.address, bar.size / MB);

    // IOBASE
    PciGetBar(&bar, pci->id, 4);
    pci->iobase = bar.u.port;
    RlogPrint("    IOBASE:       0x%X (%u bytes)\n", bar.u.port, bar.size);
}

// ------------------------------------------------------------------------------------------------
u32 GfxRead32(GfxPci *pci, uint reg)
{
    return MmioRead32((u8 *)pci->mmioBar + reg);
}

// ------------------------------------------------------------------------------------------------
u64 GfxRead64(GfxPci *pci, uint reg)
{
    return MmioRead64((u8 *)pci->mmioBar + reg);
}

// ------------------------------------------------------------------------------------------------
void GfxWrite32(GfxPci *pci, uint reg, u32 value)
{
    MmioWrite32((u8 *)pci->mmioBar + reg, value);
}

// ------------------------------------------------------------------------------------------------
void GfxWrite64(GfxPci *pci, uint reg, u32 value)
{
    MmioWrite64((u8 *)pci->mmioBar + reg, value);
}
