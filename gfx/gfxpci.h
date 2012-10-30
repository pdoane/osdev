// ------------------------------------------------------------------------------------------------
// gfx/gfxpci.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

typedef struct GfxPCI
{
    uint    id;

    void   *aperture_bar;
    void   *mmio_bar;
    u32    *gtt_addr;
    u16     iobase;

    u32    aperture_size;
} GfxPCI;

void gfx_init_pci(GfxPCI *pPci);

u32 gfx_read32(GfxPCI *pPci, uint reg);
u64 gfx_read64(GfxPCI *pPci, uint reg);
void gfx_write32(GfxPCI *pPci, uint reg, u32 value);
void gfx_write64(GfxPCI *pPci, uint reg, u32 value);
