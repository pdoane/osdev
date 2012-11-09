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

        RlogPrint("  %08x\n", *(u32 *)ring->statusPage.cpuAddr);
    }
    ExitForceWake();
}

// ------------------------------------------------------------------------------------------------
void GfxInitRing(GfxRing *ring, GfxMemManager *memMgr)
{
    uint csMemSize = 4 * KB;
    GfxAlloc(memMgr, &ring->cmdStream, csMemSize, 4 * KB);
    ring->tail = ring->cmdStream.cpuAddr;

    GfxAlloc(memMgr, &ring->statusPage, 4 * KB, 4 * KB);
    memset(ring->statusPage.cpuAddr, 0, 4 * KB);
}

// ------------------------------------------------------------------------------------------------
u32 *GfxBeginCmd(GfxRing *ring, uint dwordCount)
{
    u32 *result = (u32 *)ring->tail;
    // TODO - check for wrap
    return result;
}

// ------------------------------------------------------------------------------------------------
void GfxEndCmd(GfxPci *pci, GfxRing *ring, u32* tail)
{
    ring->tail = (u8 *)tail;
    u32 tailIndex = ring->tail - ring->cmdStream.cpuAddr;

    EnterForceWake();
    {
        GfxWrite32(pci, RCS_RING_BUFFER_TAIL, tailIndex);
        RlogPrint("...tail updated\n");
    }
    ExitForceWake();
}
