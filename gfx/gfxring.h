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
void *GfxAllocCmd(GfxRing *ring, uint cmdSize);
void GfxWriteCmd(GfxPci *pci, GfxRing *ring, uint cmdSize);
