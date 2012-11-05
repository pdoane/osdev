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
    GfxAddress top;         // Non-inclusive
    GfxAddress current;
} GfxMemRange;

// ------------------------------------------------------------------------------------------------
typedef struct GfxMemManager
{
    GfxMemRange vram;      // Stolen Memory
    GfxMemRange shared;    // Addresses mapped through aperture.
    GfxMemRange private;   // Only accessable by GPU, but allocated by CPU.

    // TEMP
    u8     *gfxMemBase;
    u8     *gfxMemNext;
} GfxMemManager;

// ------------------------------------------------------------------------------------------------
void GfxInitMemManager(GfxMemManager *memMgr, const GfxGTT *gtt, GfxPci *pci);
void GfxMemEnableSwizzle(GfxPci *pci);

u32 GfxAddr(GfxMemManager *memMgr, void *phyAddr);
void *GfxAlloc(GfxMemManager *memMgr, uint size, uint align);
