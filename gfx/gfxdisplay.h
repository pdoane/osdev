// ------------------------------------------------------------------------------------------------
// gfx/gfxdisplay.h
//
// VGA and Mode Change Function
// ------------------------------------------------------------------------------------------------

#pragma once

#include "gfx/gfxpci.h"

typedef struct GfxDisplay
{
    // MWDD FIX: TO DO
    int dummy;
} GfxDisplay;

void gfx_init_display(GfxDisplay *pDisplay);
void gfx_disable_vga(GfxPCI *pPci);