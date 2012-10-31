// ------------------------------------------------------------------------------------------------
// gfx/gfxmem.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "gfx/reg.h"
#include "gfx/gtt.h"

typedef struct GfxMemRange
{
    GfxAddress base;
    GfxAddress top;
    GfxAddress current;
} GfxMemRange;

typedef struct GfxMemManager
{
    GfxMemRange vram;      // Stollen Memory
    GfxMemRange shared;    // Addresses mapped through aperture.
    GfxMemRange private;   // Only accessable by GPU, but allocated by CPU.
} GfxMemManager;

void gfx_init_mem_manager(GfxMemManager *pMemMgr, const GfxGTT *pGTT, GfxPCI *pPci);
void gfx_mem_enable_swizzle(GfxPCI *pPci);