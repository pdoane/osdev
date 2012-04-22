// ------------------------------------------------------------------------------------------------
// kernel.c
// ------------------------------------------------------------------------------------------------

#include "console.h"
#include "idt.h"
#include "keyboard.h"
#include "pic.h"
#include "string.h"
#include "vga.h"
#include "vm.h"

// ------------------------------------------------------------------------------------------------
extern char __bss_start, __bss_end;

// ------------------------------------------------------------------------------------------------
int kmain()
{
    memset(&__bss_start, 0, &__bss_end - &__bss_start);

    vga_text_init();
    console_init();
    console_print("Welcome!\n");

    idt_init();
    vm_init();
    pic_init();
    keyboard_init();

    for (;;)
    {
        keyboard_poll();
    }

    return 0;
}
