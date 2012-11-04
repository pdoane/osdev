// ------------------------------------------------------------------------------------------------
// gfx/gfxmem.c
// ------------------------------------------------------------------------------------------------

#include "gfx/gfxmem.h"
#include "net/rlog.h"

// ------------------------------------------------------------------------------------------------
void GfxInitMemManager(GfxMemManager *memMgr, const GfxGTT *gtt, GfxPCI *pci)
{
    memMgr->vram.base       = 0;
    memMgr->vram.current    = memMgr->vram.base;
    memMgr->vram.top        = gtt->stolenMemSize - 1;

    memMgr->shared.base     = gtt->stolenMemSize;
    memMgr->shared.current  = memMgr->shared.base;
    memMgr->shared.top      = (gtt->numMappableEntries << GTT_PAGE_SHIFT) - 1;


    memMgr->private.base    = gtt->numMappableEntries << GTT_PAGE_SHIFT;
    memMgr->private.current = memMgr->private.base;
    memMgr->private.top     = (((u64)gtt->numTotalEntries) << GTT_PAGE_SHIFT) - 1;

    // Clear all fence registers (provide linear access to mem to cpu)
    for (u8 fenceNum = 0; fenceNum < FENCE_COUNT; ++fenceNum)
    {
        GfxWrite64(pci, FENCE_BASE + sizeof(RegFence) * fenceNum, 0);
    }
}

// ------------------------------------------------------------------------------------------------
void GfxMemEnableSwizzle(GfxPCI *pci)
{
    // Assume Dimms are same size, so bit 6 swizzling is on.
    // So enable DRAM swizzling.
    RegArbCtl  arbCtrl;
    RegTileCtl tileCtrl;
    RegArbMode arbMode; 

    arbCtrl.dword = GfxRead32(pci, ARB_CTL);
    arbCtrl.bits.tiledAddressSwizzling = 1;
    GfxWrite32(pci, ARB_CTL, arbCtrl.dword);

    tileCtrl.dword = GfxRead32(pci, TILE_CTL);
    tileCtrl.bits.swzctl = 1;
    GfxWrite32(pci, TILE_CTL, tileCtrl.dword);

    arbMode.dword = 0;
    arbMode.bits.data.as4ts = 1;
    arbMode.bits.mask.as4ts = 1;
    GfxWrite32(pci, ARB_MODE, arbMode.dword);

    // Tile Control may not be implemented.  It's not in the docs
    // and writes don't seem to work
    RlogPrint("ARB_CTL: 0x%08X\n", GfxRead32(pci, ARB_CTL));
    RlogPrint("TILECTL: 0x%08X\n", GfxRead32(pci, TILE_CTL));
    RlogPrint("ARB_MODE: 0x%08X\n", GfxRead32(pci, ARB_MODE));
}
