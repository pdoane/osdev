// ------------------------------------------------------------------------------------------------
// gfx/gfx.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "gfx/gfxpci.h"
#include "gfx/gfxmem.h"

// ------------------------------------------------------------------------------------------------
typedef struct GfxRing
{
    u8 *cmdStream;
    u8 *statusPage;
    u8 *tail;
} GfxRing;

// ------------------------------------------------------------------------------------------------
void GfxPrintRingState(GfxPci *pci, GfxRing *ring);
void GfxInitRing(GfxRing *ring, GfxMemManager *memMgr);
u32 *GfxBeginCmd(GfxRing *ring, uint dwordCount);
void GfxEndCmd(GfxPci *pci, GfxRing *ring, u32* tail);
