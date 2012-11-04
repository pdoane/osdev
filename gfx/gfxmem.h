// ------------------------------------------------------------------------------------------------
// gfx/gfxmem.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "gfx/reg.h"
#include "gfx/gtt.h"

// ------------------------------------------------------------------------------------------------
typedef struct GfxMemRange
{
    GfxAddress base;
    GfxAddress top;
    GfxAddress current;
} GfxMemRange;

// ------------------------------------------------------------------------------------------------
typedef struct GfxMemManager
{
    GfxMemRange vram;      // Stollen Memory
    GfxMemRange shared;    // Addresses mapped through aperture.
    GfxMemRange private;   // Only accessable by GPU, but allocated by CPU.
} GfxMemManager;

// ------------------------------------------------------------------------------------------------
void GfxInitMemManager(GfxMemManager *memMgr, const GfxGTT *gtt, GfxPCI *pci);
void GfxMemEnableSwizzle(GfxPCI *pci);
