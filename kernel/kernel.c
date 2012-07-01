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

    vga_text_init();
    console_init();
    console_print("Welcome!\n");

    vm_init();
    acpi_init();
    intr_init();
    pci_init();
    net_init();
    smp_init();

    for (;;)
    {
        usb_poll();
        net_poll();
        gfx_poll();
    }

    return 0;
}
