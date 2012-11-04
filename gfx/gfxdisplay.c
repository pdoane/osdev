// ------------------------------------------------------------------------------------------------
// gfx/gfxdisplay.c
// ------------------------------------------------------------------------------------------------

#include "gfx/gfxdisplay.h"
#include "gfx/reg.h"
#include "cpu/io.h"
#include "time/pit.h"
#include "net/rlog.h"

// ------------------------------------------------------------------------------------------------
void GfxInitDisplay(GfxDisplay *display)
{
    // MWDD FIX: To Do
    display->dummy = 0;
}

// ------------------------------------------------------------------------------------------------
void GfxDisableVga(GfxPCI *pci)
{
    IoWrite8(SR_INDEX, SEQ_CLOCKING);
    IoWrite8(SR_DATA, IoRead8(SR_DATA) | SCREEN_OFF);
    PitWait(100);
    GfxWrite32(pci, VGA_CONTROL, VGA_DISABLE);

    RlogPrint("VGA Plane disabled\n");
}
