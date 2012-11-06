// ------------------------------------------------------------------------------------------------
// gfx/gtt.c
// ------------------------------------------------------------------------------------------------

#include "gfx/gtt.h"
#include "pci/driver.h"
#include "net/rlog.h"

// ------------------------------------------------------------------------------------------------
static const u32 GMS_TO_SIZE[] =
{
      0 * MB,     // GMS_0MB
     32 * MB,     // GMS_32MB_1
     64 * MB,     // GMS_64MB_1
     96 * MB,     // GMS_96MB_1
    128 * MB,     // GMS_128MB_1
     32 * MB,     // GMS_32MB
     48 * MB,     // GMS_48MB
     64 * MB,     // GMS_64MB
    128 * MB,     // GMS_128MB
    256 * MB,     // GMS_256MB
     96 * MB,     // GMS_96MB
    160 * MB,     // GMS_160MB
    224 * MB,     // GMS_224MB
    352 * MB,     // GMS_352MB
    448 * MB,     // GMS_448MB
    480 * MB,     // GMS_480MB
    512 * MB,     // GMS_512MB
};

// ------------------------------------------------------------------------------------------------
void GfxInitGtt(GfxGTT *gtt, const GfxPci *pci)
{
    u16 ggc = PciRead16(pci->id, MGGC0);
    u32 bdsm = PciRead32(pci->id, BDSM);

    uint gms = (ggc >> GGC_GMS_SHIFT) & GGC_GMS_MASK;
    gtt->stolenMemSize = GMS_TO_SIZE[gms];
    
    uint ggms = (ggc >> GGC_GGMS_SHIFT) & GGC_GGMS_MASK;

    switch (ggms)
    {
    case GGMS_None:
        gtt->gttMemSize = 0;
        break;

    case GGMS_1MB:
        gtt->gttMemSize = 1 * MB;
        break;

    case GGMS_2MB:
        gtt->gttMemSize = 2 * MB;
        break;

    default:
        gtt->gttMemSize = -1;
        break;
    }

    gtt->stolenMemBase = bdsm & BDSM_ADDR_MASK;

    gtt->numTotalEntries    = gtt->gttMemSize / sizeof(u32);
    gtt->numMappableEntries = pci->apertureSize >> GTT_PAGE_SHIFT;
    gtt->entries = pci->gttAddr;

    // MWDD FIX: Should map unused pages to a safe page.

    RlogPrint("\n");
    RlogPrint("...GTT Config\n");
    RlogPrint("    Stolen Mem Base:      %p\n",    gtt->stolenMemBase);
    RlogPrint("    Stolen Mem Size:      %d MB\n", gtt->stolenMemSize / MB);
    RlogPrint("    GTT Mem Size:         %d MB\n", gtt->gttMemSize / MB);
    RlogPrint("    GTT Total Entries:    %d\n",    gtt->numTotalEntries);
    RlogPrint("    GTT Mappable Entries: %d\n",    gtt->numMappableEntries);
}
