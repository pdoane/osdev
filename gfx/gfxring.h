// ------------------------------------------------------------------------------------------------
// gfx/gfx.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "gfx/gfxpci.h"
#include "gfx/gfxmem.h"

// ------------------------------------------------------------------------------------------------
typedef enum GfxRingId
{
    RING_RCS,
    RING_VCS,
    RING_BCS,
    RING_COUNT
} GfxRingId;

// ------------------------------------------------------------------------------------------------
typedef struct GfxRing
{
    GfxRingId id;
    GfxObject cmdStream;
    GfxObject statusPage;
    u8 *tail;
} GfxRing;

// ------------------------------------------------------------------------------------------------
void GfxPrintRingState(GfxPci *pci, GfxRing *ring);
void GfxInitRing(GfxRing *ring, GfxRingId id, GfxMemManager *memMgr);
void GfxSetRing(GfxPci *pci, GfxRing *ring);

u32 *GfxBeginCmd(GfxRing *ring, uint dwordCount);
void GfxEndCmd(GfxPci *pci, GfxRing *ring, u32* tail);
