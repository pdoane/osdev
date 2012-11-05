// ------------------------------------------------------------------------------------------------
// gfx/gfxring.c
// ------------------------------------------------------------------------------------------------

#include "gfx/gfxring.h"
#include "gfx/gfxmem.h"
#include "gfx/gfxpci.h"
#include "net/rlog.h"
#include "stdlib/string.h"

// TEMP
void EnterForceWake();
void ExitForceWake();

// ------------------------------------------------------------------------------------------------
void GfxPrintRingState(GfxPci *pci, GfxRing *ring)
{
    EnterForceWake();
    {
        RlogPrint("  RCS_HWS_PGA: 0x%08X\n", GfxRead32(pci, RCS_HWS_PGA));

        RlogPrint("  RCS_RING_BUFFER_TAIL: 0x%08X\n", GfxRead32(pci, RCS_RING_BUFFER_TAIL));
        RlogPrint("  RCS_RING_BUFFER_HEAD: 0x%08X\n", GfxRead32(pci, RCS_RING_BUFFER_HEAD));
        RlogPrint("  RCS_RING_BUFFER_START: 0x%08X\n", GfxRead32(pci, RCS_RING_BUFFER_START));
        RlogPrint("  RCS_RING_BUFFER_CTL: 0x%08X\n", GfxRead32(pci, RCS_RING_BUFFER_CTL));

        RlogPrint("  %08x\n", *(u32 *)ring->statusPage);
    }
    ExitForceWake();
}

// ------------------------------------------------------------------------------------------------
void GfxInitRing(GfxRing *ring, GfxMemManager *memMgr)
{
    uint csMemSize = 4 * KB;
    ring->cmdStream = GfxAlloc(memMgr, csMemSize, 4 * KB);
    ring->tail = ring->cmdStream;

    ring->statusPage = GfxAlloc(memMgr, 4 * KB, 4 * KB);
    memset(ring->statusPage, 0, 4 * KB);
}

// ------------------------------------------------------------------------------------------------
void *GfxAllocCmd(GfxRing *ring, uint cmdSize)
{
    void *result = ring->tail;
    memset(result, 0, cmdSize);
    return result;
}

// ------------------------------------------------------------------------------------------------
void GfxWriteCmd(GfxPci *pci, GfxRing *ring, uint cmdSize)
{
    ring->tail += cmdSize;
    u32 tail = ring->tail - ring->cmdStream;

    EnterForceWake();
    {
        GfxWrite32(pci, RCS_RING_BUFFER_TAIL, tail);
        RlogPrint("...tail updated\n");
    }
    ExitForceWake();
}
