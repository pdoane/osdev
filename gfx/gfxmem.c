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
        GfxWrite64(pci, FENCE_BASE + sizeof(RegFence) * fenceNum, 0);
    }
}

// ------------------------------------------------------------------------------------------------
void GfxMemEnableSwizzle(GfxPCI *pci)
{
    // Only enable swizzling when DIMMs are the same size.
    RegMadDIMM dimmCh0;
    RegMadDIMM dimmCh1;

    dimmCh0.dword = GfxRead32(pci, GFX_MCHBAR + MAD_DIMM_CH0);
    dimmCh1.dword = GfxRead32(pci, GFX_MCHBAR + MAD_DIMM_CH1);
    RlogPrint("dimmCh0: 0x%08X\n", dimmCh0);
    RlogPrint("dimmCh1: 0x%08X\n", dimmCh1);
    if (dimmCh0.bits.dimmASize != dimmCh1.bits.dimmASize ||
        dimmCh0.bits.dimmBSize != dimmCh1.bits.dimmBSize)
    {
        return;
    }

    // Enable Bit 6 Swizzling
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

    RlogPrint("ARB_CTL: 0x%08X\n", GfxRead32(pci, ARB_CTL));
    RlogPrint("TILE_CTL: 0x%08X\n", GfxRead32(pci, TILE_CTL));
    RlogPrint("ARB_MODE: 0x%08X\n", GfxRead32(pci, ARB_MODE));
}
