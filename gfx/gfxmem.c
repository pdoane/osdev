// ------------------------------------------------------------------------------------------------
// gfx/gfxmem.c
// ------------------------------------------------------------------------------------------------

#include "gfx/gfxmem.h"
#include "net/rlog.h"


void gfx_init_mem_manager(GfxMemManager *pMemMgr, const GfxGTT *pGTT, GfxPCI *pPCI)
{
    pMemMgr->vram.base       = 0;
    pMemMgr->vram.current    = pMemMgr->vram.base;
    pMemMgr->vram.top        = pGTT->stolenMemSize - 1;

    pMemMgr->shared.base     = pGTT->stolenMemSize;
    pMemMgr->shared.current  = pMemMgr->shared.base;
    pMemMgr->shared.top      = (pGTT->numMappableEntries << GTT_PAGE_SHIFT) - 1;


    pMemMgr->private.base    = pGTT->numMappableEntries << GTT_PAGE_SHIFT;
    pMemMgr->private.current = pMemMgr->private.base;
    pMemMgr->private.top     = (((u64)pGTT->numTotalEntries) << GTT_PAGE_SHIFT) - 1;

    // Clear all fence registers (provide linear access to mem to cpu)
    for (u8 fenceNum = 0; fenceNum < FENCE_COUNT; ++fenceNum)
    {
        gfx_write64(pPCI, FENCE_BASE + sizeof(RegFence) * fenceNum, 0);
    }
}

void gfx_mem_enable_swizzle(GfxPCI *pPci)
{
    // Assume Dimms are same size, so bit 6 swizzling is on.
    // So enable DRAM swizzling.
    RegArbCtl  arbCtrl;
    RegTileCtl tileCtrl;
    RegArbMode arbMode; 

    arbCtrl.dword = gfx_read32(pPci, ARB_CTL);
    arbCtrl.bits.tiledAddressSwizzling = 1;
    gfx_write32(pPci, ARB_CTL, arbCtrl.dword);

    tileCtrl.dword = gfx_read32(pPci, TILE_CTL);
    tileCtrl.bits.swzctl = 1;
    gfx_write32(pPci, TILE_CTL, tileCtrl.dword);

    arbMode.dword = 0;
    arbMode.bits.data.as4ts = 1;
    arbMode.bits.mask.as4ts = 1;
    gfx_write32(pPci, ARB_MODE, arbMode.dword);

    // Tile Control may not be implemented.  It's not in the docs
    // and writes don't seem to work
    rlog_print("ARB_CTL: 0x%08X\n", gfx_read32(pPci, ARB_CTL));
    rlog_print("TILECTL: 0x%08X\n", gfx_read32(pPci, TILE_CTL));
    rlog_print("ARB_MODE: 0x%08X\n", gfx_read32(pPci, ARB_MODE));
}