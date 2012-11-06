// ------------------------------------------------------------------------------------------------
// gfx/gtt.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "gfx/reg.h"
#include "gfx/gfxpci.h"

// ------------------------------------------------------------------------------------------------
typedef struct GfxGTT
{
    // Config from PCI config space
    u32 stolenMemSize;
    u32 gttMemSize;
    u32 stolenMemBase;

    u32 numTotalEntries;     // How many entries in the GTT
    u32 numMappableEntries;  // How many can be mapped at once

    u32 *entries;
} GfxGTT;

// ------------------------------------------------------------------------------------------------
void GfxInitGtt(GfxGTT *gtt, const GfxPci *pci);
