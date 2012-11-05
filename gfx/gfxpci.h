// ------------------------------------------------------------------------------------------------
// gfx/gfxpci.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
typedef struct GfxPci
{
    uint    id;

    void   *apertureBar;
    void   *mmioBar;
    u32    *gttAddr;
    u16     iobase;

    u32     apertureSize;
} GfxPci;

// ------------------------------------------------------------------------------------------------
void GfxInitPci(GfxPci *pci);

u32 GfxRead32(GfxPci *pci, uint reg);
u64 GfxRead64(GfxPci *pci, uint reg);
void GfxWrite32(GfxPci *pci, uint reg, u32 value);
void GfxWrite64(GfxPci *pci, uint reg, u32 value);
