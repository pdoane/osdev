// ------------------------------------------------------------------------------------------------
// pic.c
// ------------------------------------------------------------------------------------------------

#include "pic.h"
#include "int.h"
#include "io.h"

// ------------------------------------------------------------------------------------------------
#define PIC1_CMD                        0x0020
#define PIC1_DATA                       0x0021
#define PIC2_CMD                        0x00a0
#define PIC2_DATA                       0x00a1

#define ICW1_ICW4                       0x01        // ICW4 command word: 0 = not needed, 1 = needed
#define ICW1_SINGLE                     0x02        // Single mode: 0 = cascade, 1 = single
#define ICW1_ADI                        0x04        // Call address interval: 0 = interval of 8, 1 = interval of 4
#define ICW1_LTIM                       0x08        // Interrupt trigger mode: 0 = edge, 1 = level
#define ICW1_INIT                       0x10        // Initialization

#define ICW4_8086                       0x01        // Microprocessor mode: 0=MCS-80/85, 1=8086/8088
#define ICW4_AUTO                       0x02        // Auto EOI: 0 = disabled, 1 = enabled
#define ICW4_BUF_SLAVE                  0x04        // Buffered mode/slave
#define ICW4_BUF_MASTER                 0x0C        // Buffered mode/master
#define ICW4_SFNM                       0x10        // Special fully nested is programmed

// ------------------------------------------------------------------------------------------------
void pic_init()
{
    // ICW1: start initialization, ICW4 needed
    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);

    // ICW2: interrupt vector address
    outb(PIC1_DATA, IRQ_BASE);
    outb(PIC2_DATA, IRQ_BASE + 8);

    // ICW3: master/slave wiring
    outb(PIC1_DATA, 4);
    outb(PIC2_DATA, 2);

    // ICW4: 8086 mode, not special fully nested, not buffered, normal EOI
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // OCW1: Disable all IRQs
    outb(PIC1_DATA, 0xff);
    outb(PIC2_DATA, 0xff);
}
