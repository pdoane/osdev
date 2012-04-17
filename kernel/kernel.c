// ------------------------------------------------------------------------------------------------
// kernel.c
// ------------------------------------------------------------------------------------------------

#include "kernel.h"
#include "format.h"

typedef struct MemoryRegion
{
    u64 start;
    u64 size;
    u32 type;
    u32 acpi_3_0;
} MemoryRegion;

void keyboard_interrupt();

// ------------------------------------------------------------------------------------------------
static void clear()
{
    volatile u64* p = (volatile u64*)VGA_TEXT_BASE;
    for (int i = 0; i < 500; ++i)
    {
        *p++ = 0x1f201f201f201f20;
    }
}

// ------------------------------------------------------------------------------------------------
static void print(const char* s, uint line)
{
    volatile u8* p = (volatile u8*)VGA_TEXT_BASE;

    p += line * 80 * 2;

    while (*s)
    {
        *p++ = *s++;
        *p++ = 0x1f;
    }
}

// ------------------------------------------------------------------------------------------------
static void dump_memory_map()
{
    char buf[80];

    uint line = 0;
    MemoryRegion* region = (MemoryRegion*)MEMORY_MAP;
    while (region->type)
    {
        snprintf(buf, sizeof(buf), "region %d: start: 0x%016llx end: 0x%016llx type: %d", line,
            region->start, region->start + region->size, region->type);
        print(buf, line);

        ++region;
        ++line;
    }
}

// ------------------------------------------------------------------------------------------------
int kmain()
{
    clear();
    idt_init();
    dump_memory_map();

    // keyboard test code

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

    // Enable keyboard interrupt
    idt_set_handler(0x21, INTERRUPT_GATE, keyboard_interrupt);
    outb(0x0021, inb(0x0021) & ~0x2);

    for (;;)
    {
    }

    return 0;
}
