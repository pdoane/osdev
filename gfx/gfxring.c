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
// Ring register macros

#define HWS_PGA(id)                     (RCS_HWS_PGA + ((id) << 8))
#define RING_BUFFER_TAIL(id)            (RCS_RING_BUFFER_TAIL + ((id) << 16))
#define RING_BUFFER_HEAD(id)            (RCS_RING_BUFFER_HEAD + ((id) << 16))
#define RING_BUFFER_START(id)           (RCS_RING_BUFFER_START + ((id) << 16))
#define RING_BUFFER_CTL(id)             (RCS_RING_BUFFER_CTL + ((id) << 16))

// ------------------------------------------------------------------------------------------------
void GfxPrintRingState(GfxPci *pci, GfxRing *ring)
{
    EnterForceWake();
    {
        RlogPrint("  Ring Id: %d\n", ring->id);
        RlogPrint("  HWS_PGA: 0x%08X\n", GfxRead32(pci, HWS_PGA(ring->id)));

        RlogPrint("  RING_BUFFER_TAIL: 0x%08X\n", GfxRead32(pci, RING_BUFFER_TAIL(ring->id)));
        RlogPrint("  RING_BUFFER_HEAD: 0x%08X\n", GfxRead32(pci, RING_BUFFER_HEAD(ring->id)));
        RlogPrint("  RING_BUFFER_START: 0x%08X\n", GfxRead32(pci, RING_BUFFER_START(ring->id)));
        RlogPrint("  RING_BUFFER_CTL: 0x%08X\n", GfxRead32(pci, RING_BUFFER_CTL(ring->id)));

        RlogPrint("  %08x\n", *(u32 *)ring->statusPage.cpuAddr);
    }
    ExitForceWake();
}

// ------------------------------------------------------------------------------------------------
void GfxInitRing(GfxRing *ring, GfxRingId id, GfxMemManager *memMgr)
{
    uint csMemSize = 4 * KB;
    GfxAlloc(memMgr, &ring->cmdStream, csMemSize, 4 * KB);
    GfxAlloc(memMgr, &ring->statusPage, 4 * KB, 4 * KB);

    ring->id = id;
    ring->tail = ring->cmdStream.cpuAddr;
    memset(ring->statusPage.cpuAddr, 0, 4 * KB);
}

// ------------------------------------------------------------------------------------------------
void GfxSetRing(GfxPci *pci, GfxRing *ring)
{
    EnterForceWake();
    {
        RlogPrint("Setting Ring...\n");

        // Setup Render Ring Buffer
        GfxWrite32(pci, HWS_PGA(ring->id), ring->statusPage.gfxAddr);
        GfxWrite32(pci, RING_BUFFER_TAIL(ring->id), 0);
        GfxWrite32(pci, RING_BUFFER_HEAD(ring->id), 0);
        GfxWrite32(pci, RING_BUFFER_START(ring->id), ring->cmdStream.gfxAddr);
        GfxWrite32(pci, RING_BUFFER_CTL(ring->id),
              (0 << 12)         // # of pages - 1
            | 1                 // Ring Buffer Enable
            );
        RlogPrint("...done\n");

    }
    ExitForceWake();
}

// ------------------------------------------------------------------------------------------------
u32 *GfxBeginCmd(GfxRing *ring, uint dwordCount)
{
    u32 *result = (u32 *)ring->tail;
    // TODO - check for wrap
    return result;
}

// ------------------------------------------------------------------------------------------------
void GfxEndCmd(GfxPci *pci, GfxRing *ring, u32 *tail)
{
    ring->tail = (u8 *)tail;
    u32 tailIndex = ring->tail - ring->cmdStream.cpuAddr;

    EnterForceWake();
    {
        GfxWrite32(pci, RING_BUFFER_TAIL(ring->id), tailIndex);
        RlogPrint("...tail updated\n");
    }
    ExitForceWake();
}
