// ------------------------------------------------------------------------------------------------
// gfx/gfxpci.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
typedef struct GfxPCI
{
    uint    id;

    void   *apertureBar;
    void   *mmioBar;
    u32    *gttAddr;
    u16     iobase;

    u32     apertureSize;
} GfxPCI;

// ------------------------------------------------------------------------------------------------
void GfxInitPci(GfxPCI *pci);

u32 GfxRead32(GfxPCI *pci, uint reg);
u64 GfxRead64(GfxPCI *pci, uint reg);
void GfxWrite32(GfxPCI *pci, uint reg, u32 value);
void GfxWrite64(GfxPCI *pci, uint reg, u32 value);
