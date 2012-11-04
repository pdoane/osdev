// ------------------------------------------------------------------------------------------------
// intr/idt.c
// ------------------------------------------------------------------------------------------------

#include "intr/idt.h"
#include "mem/lowmem.h"

// ------------------------------------------------------------------------------------------------
typedef struct IdtDesc
{
    u16 limit;
    u64 base;
} PACKED IdtDesc;

typedef struct IdtEntry
{
    u16 offset0;
    u16 selector;
    u16 type;
    u16 offset1;
    u32 offset2;
    u32 reserved;
} PACKED IdtEntry;

typedef void (*Handler)();

extern void default_exception_handler();
extern void default_interrupt_handler();
extern void (*exception_handlers[20])();

// ------------------------------------------------------------------------------------------------
void IdtInit()
{
    // Build initial IDT
    for (uint i = 0; i < 20; ++i)
    {
        IdtSetHandler(i, INTERRUPT_GATE, exception_handlers[i]);
    }

    for (uint i = 20; i < 32; ++i)
    {
        IdtSetHandler(i, INTERRUPT_GATE, default_exception_handler);
    }

    for (uint i = 32; i < 256; ++i)
    {
        IdtSetHandler(i, TRAP_GATE, default_interrupt_handler);
    }

    IdtDesc idtDesc =
    {
        .limit = 256 * sizeof(IdtEntry) - 1,
        .base = IDT_BASE
    };
    __asm__ volatile("lidt %0" : : "m" (idtDesc) : "memory");

    // Test interrupt
    // __asm__ volatile("int $3");
}

// ------------------------------------------------------------------------------------------------
static void IdtSetEntry(u8 index, u64 base, u16 selector, u16 type)
{
    IdtEntry *entry = (IdtEntry *)IDT_BASE + index;

    entry->offset0 = (u16)base;
    entry->selector = selector;
    entry->type = type;
    entry->offset1 = (u16)(base >> 16);
    entry->offset2 = (u32)(base >> 32);
    entry->reserved = 0;
}

// ------------------------------------------------------------------------------------------------
void IdtSetHandler(u8 index, u16 type, void (*handler)())
{
    if (handler)
    {
        u16 selector = 0x8; // gdt.code
        IdtSetEntry(index, (u64)handler, selector, type);
    }
    else
    {
        IdtSetEntry(index, 0, 0, 0);
    }
}
