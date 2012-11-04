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
void IntrInit()
{
    // Build Interrupt Table
    IdtInit();
    IdtSetHandler(INT_TIMER, INTERRUPT_GATE, pit_interrupt);
    IdtSetHandler(INT_SPURIOUS, INTERRUPT_GATE, spurious_interrupt);

    // Initialize subsystems
    PicInit();
    LocalApicInit();
    IoApicInit();
    PitInit();

    // Enable IO APIC entries
    IoApicSetEntry(g_ioApicAddr, AcpiRemapIrq(IRQ_TIMER), INT_TIMER);

    // Enable all interrupts
    __asm__ volatile("sti");
}
