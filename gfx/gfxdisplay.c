// ------------------------------------------------------------------------------------------------
// gfx/gfxdisplay.c
// ------------------------------------------------------------------------------------------------

#include "gfx/gfxdisplay.h"
#include "gfx/reg.h"
#include "cpu/io.h"
#include "time/pit.h"
#include "net/rlog.h"


void gfx_init_display(GfxDisplay *pDisplay)
{
    // MWDD FIX: To Do
    pDisplay->dummy = 0;
}

void gfx_disable_vga(GfxPCI *pPci)
{
    out8(SR_INDEX, SEQ_CLOCKING);
    out8(SR_DATA, in8(SR_DATA) | SCREEN_OFF);
    pit_wait(100);
    gfx_write32(pPci, VGA_CONTROL, VGA_DISABLE);

    rlog_print("VGA Plane disabled\n");
}