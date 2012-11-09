// ------------------------------------------------------------------------------------------------
// gfx/gfxmem.c
// ------------------------------------------------------------------------------------------------

#include "gfx/gfxmem.h"
#include "net/rlog.h"

// ------------------------------------------------------------------------------------------------
void GfxInitMemManager(GfxMemManager *memMgr, const GfxGTT *gtt, GfxPci *pci)
{
    memMgr->vram.base       = 0;
    memMgr->vram.current    = memMgr->vram.base;
    memMgr->vram.top        = gtt->stolenMemSize;

    memMgr->shared.base     = gtt->stolenMemSize;
    memMgr->shared.current  = memMgr->shared.base;
    memMgr->shared.top      = gtt->numMappableEntries << GTT_PAGE_SHIFT;

    memMgr->private.base    = gtt->numMappableEntries << GTT_PAGE_SHIFT;
    memMgr->private.current = memMgr->private.base;
    memMgr->private.top     = ((u64)gtt->numTotalEntries) << GTT_PAGE_SHIFT;

    // Clear all fence registers (provide linear access to mem to cpu)
    for (u8 fenceNum = 0; fenceNum < FENCE_COUNT; ++fenceNum)
    {
        GfxWrite64(pci, FENCE_BASE + sizeof(u64) * fenceNum, 0);
    }

    // TEMP
    memMgr->gfxMemBase = pci->apertureBar;
    memMgr->gfxMemNext = memMgr->gfxMemBase + 4 * GTT_PAGE_SIZE;
}

// ------------------------------------------------------------------------------------------------
void GfxMemEnableSwizzle(GfxPci *pci)
{
    // Only enable swizzling when DIMMs are the same size.
    u32 dimmCh0 = GfxRead32(pci, GFX_MCHBAR + MAD_DIMM_CH0);
    u32 dimmCh1 = GfxRead32(pci, GFX_MCHBAR + MAD_DIMM_CH1);
    RlogPrint("dimmCh0: 0x%08X\n", dimmCh0);
    RlogPrint("dimmCh1: 0x%08X\n", dimmCh1);
    if ((dimmCh0 & MAD_DIMM_AB_SIZE_MASK) != (dimmCh1 & MAD_DIMM_AB_SIZE_MASK))
    {
        return;
    }

    // Enable Bit 6 Swizzling
    u32 arbCtl = GfxRead32(pci, ARB_CTL);
    arbCtl |= ARB_CTL_TILED_ADDRESS_SWIZZLING;
    GfxWrite32(pci, ARB_CTL, arbCtl);

    u32 tileCtl = GfxRead32(pci, TILE_CTL);
    tileCtl |= TILE_CTL_SWIZZLE;
    GfxWrite32(pci, TILE_CTL, tileCtl);

    GfxWrite32(pci, ARB_MODE, MASKED_ENABLE(ARB_MODE_AS4TS));

    RlogPrint("ARB_CTL: 0x%08X\n", GfxRead32(pci, ARB_CTL));
    RlogPrint("TILE_CTL: 0x%08X\n", GfxRead32(pci, TILE_CTL));
    RlogPrint("ARB_MODE: 0x%08X\n", GfxRead32(pci, ARB_MODE));
}

// ------------------------------------------------------------------------------------------------
u32 GfxAddr(GfxMemManager *memMgr, void *phyAddr)
{
    return (u32)((u8*)phyAddr - memMgr->gfxMemBase);
}

// ------------------------------------------------------------------------------------------------
bool GfxAlloc(GfxMemManager *memMgr, GfxObject *obj, uint size, uint align)
{
    // Align memory request
    u8 *cpuAddr = memMgr->gfxMemNext;
    uintptr_t offset = (uintptr_t)cpuAddr & (align - 1);
    if (offset)
    {
        cpuAddr += align - offset;
    }

    memMgr->gfxMemNext = cpuAddr + size;
    obj->cpuAddr = cpuAddr;
    obj->gfxAddr = cpuAddr - memMgr->gfxMemBase;
    return true;
}
