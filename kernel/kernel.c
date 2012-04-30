// ------------------------------------------------------------------------------------------------
// kernel.c
// ------------------------------------------------------------------------------------------------

#include "console.h"
#include "acpi.h"
#include "idt.h"
#include "ioapic.h"
#include "keyboard.h"
#include "local_apic.h"
#include "int.h"
#include "pic.h"
#include "string.h"
#include "vga.h"
#include "vm.h"

// ------------------------------------------------------------------------------------------------
extern char __bss_start, __bss_end;

extern void keyboard_interrupt();
extern void spurious_interrupt();

static void interrupt_init();

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
    interrupt_init();

    for (;;)
    {
        keyboard_poll();
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
static void interrupt_init()
{
    // Build Interrupt Table
    idt_init();
    idt_set_handler(IRQ_BASE + IRQ_KEYBOARD, INTERRUPT_GATE, keyboard_interrupt);
    idt_set_handler(INT_SPURIOUS, INTERRUPT_GATE, spurious_interrupt);

    // Initialize PIC disabled
    pic_init();

    // Initialize local APIC and I/O APIC
    local_apic_init();
    ioapic_init();

    // Enable keyboard
    ioapic_set_entry(ioapic_address, IRQ_KEYBOARD, IRQ_BASE + IRQ_KEYBOARD);

    // Enable all interrupts
    __asm__ volatile("sti");
}
