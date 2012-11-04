// ------------------------------------------------------------------------------------------------
// kernel/kernel.c
// ------------------------------------------------------------------------------------------------

#include "acpi/acpi.h"
#include "console/console.h"
#include "cpu/smp.h"
#include "gfx/gfx.h"
#include "gfx/vga.h"
#include "intr/intr.h"
#include "mem/vm.h"
#include "net/net.h"
#include "pci/pci.h"
#include "stdlib/string.h"
#include "usb/usb.h"

// ------------------------------------------------------------------------------------------------
extern char __bss_start, __bss_end;

// ------------------------------------------------------------------------------------------------
// !! This function must be the first in the file.
int kmain()
{
    memset(&__bss_start, 0, &__bss_end - &__bss_start);

    VgaTextInit();
    ConsoleInit();
    ConsolePrint("Welcome!\n");

    VMInit();
    AcpiInit();
    IntrInit();
    PciInit();
    NetInit();
    SmpInit();

    for (;;)
    {
        UsbPoll();
        NetPoll();
        GfxPoll();
    }

    return 0;
}
