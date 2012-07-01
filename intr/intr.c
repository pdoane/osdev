// ------------------------------------------------------------------------------------------------
// intr/intr.c
// ------------------------------------------------------------------------------------------------

#include "intr/intr.h"
#include "intr/idt.h"
#include "intr/ioapic.h"
#include "intr/local_apic.h"
#include "intr/pic.h"
#include "acpi/acpi.h"
#include "time/pit.h"

// ------------------------------------------------------------------------------------------------
extern void pit_interrupt();
extern void spurious_interrupt();

// ------------------------------------------------------------------------------------------------
void intr_init()
{
    // Build Interrupt Table
    idt_init();
    idt_set_handler(INT_TIMER, INTERRUPT_GATE, pit_interrupt);
    idt_set_handler(INT_SPURIOUS, INTERRUPT_GATE, spurious_interrupt);

    // Initialize subsystems
    pic_init();
    lapic_init();
    ioapic_init();
    pit_init();

    // Enable IO APIC entries
    ioapic_set_entry(ioapic_address, acpi_remap_irq(IRQ_TIMER), INT_TIMER);

    // Enable all interrupts
    __asm__ volatile("sti");
}
