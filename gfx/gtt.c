// ------------------------------------------------------------------------------------------------
// gfx/gtt.c
// ------------------------------------------------------------------------------------------------

#include "gfx/gtt.h"
#include "pci/driver.h"
#include "net/rlog.h"

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


void gfx_init_gtt(GfxGTT *pGTT, const GfxPCI *pPci)
{
    RegMGGCO mggco;
    RegBDSM  bdsm;

    mggco.word = pci_in16(pPci->id, MGGC0);
    bdsm.dword = pci_in32(pPci->id, BDSM);

    pGTT->stolenMemSize = MGGCO_GMS_TO_SIZE[mggco.bits.graphicsModeSelect];
    
    switch (mggco.bits.gttMemSize)
    {
        case RegMGGCO_GGMS_None:
            pGTT->gttMemSize = 0;
            break;

        case RegMGGCO_GGMS_1MB:
            pGTT->gttMemSize = 1 * MB;
            break;

        case RegMGGCO_GGMS_2MB:
            pGTT->gttMemSize = 2 * MB;
            break;

        default:
           pGTT->gttMemSize = -1;
    }

    bdsm.bits.lock = 0;
    pGTT->stolenMemBase = bdsm.dword;

    pGTT->numTotalEntries    = pGTT->gttMemSize / sizeof(GttEntry);
    pGTT->numMappableEntries = pPci->aperture_size >> GTT_PAGE_SHIFT;
    pGTT->pGtt = (GttEntry *) pPci->gtt_addr;

    // MWDD FIX: Should map unused pages to a safe page.

    rlog_print("\n");
    rlog_print("...GTT Config\n");
    rlog_print("    Stolen Mem Base:      %p\n",    pGTT->stolenMemBase);
    rlog_print("    Stolen Mem Size:      %d MB\n", pGTT->stolenMemSize / MB);
    rlog_print("    GTT Mem Size:         %d MB\n", pGTT->gttMemSize / MB);
    rlog_print("    GTT Total Entries:    %d\n",    pGTT->numTotalEntries);
    rlog_print("    GTT Mappable Entries: %d\n",    pGTT->numMappableEntries);
}