// ------------------------------------------------------------------------------------------------
// gfx/gfxmem.c
// ------------------------------------------------------------------------------------------------

#include "gfx/gfxmem.h"

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
