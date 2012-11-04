// ------------------------------------------------------------------------------------------------
// gfx/gtt.c
// ------------------------------------------------------------------------------------------------

#include "gfx/gtt.h"
#include "pci/driver.h"
#include "net/rlog.h"

// ------------------------------------------------------------------------------------------------
static const u32 MGGCO_GMS_TO_SIZE[] =
{
      0 * MB,     // RegMGGCO_GMS_0MB
     32 * MB,     // RegMGGCO_GMS_32MB_1
     64 * MB,     // RegMGGCO_GMS_64MB_1
     96 * MB,     // RegMGGCO_GMS_96MB_1
    128 * MB,     // RegMGGCO_GMS_128MB_1
     32 * MB,     // RegMGGCO_GMS_32MB
     48 * MB,     // RegMGGCO_GMS_48MB
     64 * MB,     // RegMGGCO_GMS_64MB
    128 * MB,     // RegMGGCO_GMS_128MB
    256 * MB,     // RegMGGCO_GMS_256MB
     96 * MB,     // RegMGGCO_GMS_96MB
    160 * MB,     // RegMGGCO_GMS_160MB
    224 * MB,     // RegMGGCO_GMS_224MB
    352 * MB,     // RegMGGCO_GMS_352MB
    448 * MB,     // RegMGGCO_GMS_448MB
    480 * MB,     // RegMGGCO_GMS_480MB
    512 * MB,     // RegMGGCO_GMS_512MB
};

// ------------------------------------------------------------------------------------------------
void GfxInitGtt(GfxGTT *gtt, const GfxPCI *pci)
{
    RegMGGCO mggco;
    RegBDSM  bdsm;

    mggco.word = PciRead16(pci->id, MGGC0);
    bdsm.dword = PciRead32(pci->id, BDSM);

    gtt->stolenMemSize = MGGCO_GMS_TO_SIZE[mggco.bits.graphicsModeSelect];
    
    switch (mggco.bits.gttMemSize)
    {
        case RegMGGCO_GGMS_None:
            gtt->gttMemSize = 0;
            break;

        case RegMGGCO_GGMS_1MB:
            gtt->gttMemSize = 1 * MB;
            break;

        case RegMGGCO_GGMS_2MB:
            gtt->gttMemSize = 2 * MB;
            break;

        default:
           gtt->gttMemSize = -1;
    }

    bdsm.bits.lock = 0;
    gtt->stolenMemBase = bdsm.dword;

    gtt->numTotalEntries    = gtt->gttMemSize / sizeof(GttEntry);
    gtt->numMappableEntries = pci->apertureSize >> GTT_PAGE_SHIFT;
    gtt->gtt = (GttEntry *) pci->gttAddr;

    // MWDD FIX: Should map unused pages to a safe page.

    RlogPrint("\n");
    RlogPrint("...GTT Config\n");
    RlogPrint("    Stolen Mem Base:      %p\n",    gtt->stolenMemBase);
    RlogPrint("    Stolen Mem Size:      %d MB\n", gtt->stolenMemSize / MB);
    RlogPrint("    GTT Mem Size:         %d MB\n", gtt->gttMemSize / MB);
    RlogPrint("    GTT Total Entries:    %d\n",    gtt->numTotalEntries);
    RlogPrint("    GTT Mappable Entries: %d\n",    gtt->numMappableEntries);
}
