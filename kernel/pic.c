// ------------------------------------------------------------------------------------------------
// pic.c
// ------------------------------------------------------------------------------------------------

#include "pic.h"
#include "io.h"

// ------------------------------------------------------------------------------------------------
void pic_init()
{
    // ICW1: start initialization, ICW4 needed
    outb(0x20, 0x11);
    outb(0xa0, 0x11);

    // ICW2: interrupt vector address
    outb(0x21, 0x20);
    outb(0xa1, 0x28);

    // ICW3: master/slave wiring
    outb(0x21, 0x04);
    outb(0xa1, 0x02);

    // ICW4: 8086 mode, not special fully nested, not buffered, normal EOI
    outb(0x21, 0x01);
    outb(0xa1, 0x01);

    // OCW1: Disable all IRQs
    outb(0x21, 0xFB);
    outb(0xa1, 0xFF);

    // Enable interrupts
    __asm__ volatile("sti");
}
