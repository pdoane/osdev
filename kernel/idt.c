// ------------------------------------------------------------------------------------------------
// idt.c
// ------------------------------------------------------------------------------------------------

#include "idt.h"
#include "lowmem.h"

// ------------------------------------------------------------------------------------------------
typedef struct IDT_Desc
{
    u16 limit;
    u64 base;
} PACKED IDT_Desc;

typedef struct IDT_Entry
{
    u16 offset0;
    u16 selector;
    u16 type;
    u16 offset1;
    u32 offset2;
    u32 reserved;
} PACKED IDT_Entry;

typedef void (*Handler)();

void default_exception_handler();
void default_interrupt_handler();
void (*exception_handlers[20])();

// ------------------------------------------------------------------------------------------------
void idt_init()
{
    // Build initial IDT
    for (uint i = 0; i < 20; ++i)
    {
        idt_set_handler(i, INTERRUPT_GATE, exception_handlers[i]);
    }

    for (uint i = 20; i < 32; ++i)
    {
        idt_set_handler(i, INTERRUPT_GATE, default_exception_handler);
    }

    for (uint i = 32; i < 256; ++i)
    {
        idt_set_handler(i, TRAP_GATE, default_interrupt_handler);
    }

    IDT_Desc idt_desc =
    {
        .limit = 256 * sizeof(IDT_Entry) - 1,
        .base = IDT_BASE
    };
    __asm__ volatile("lidt %0" : : "m" (idt_desc) : "memory");

    // Test interrupt
    // __asm__ volatile("int $3");
}

// ------------------------------------------------------------------------------------------------
static void idt_set_entry(u8 index, u64 base, u16 selector, u16 type)
{
    IDT_Entry* entry = (IDT_Entry*)IDT_BASE + index;

    entry->offset0 = (u16)base;
    entry->selector = selector;
    entry->type = type;
    entry->offset1 = (u16)(base >> 16);
    entry->offset2 = (u32)(base >> 32);
    entry->reserved = 0;
}

// ------------------------------------------------------------------------------------------------
void idt_set_handler(u8 index, u16 type, void (*handler)())
{
    if (handler)
    {
        u16 selector = 0x8; // gdt.code
        idt_set_entry(index, (u64)handler, selector, type);
    }
    else
    {
        idt_set_entry(index, 0, 0, 0);
    }
}
