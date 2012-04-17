// ------------------------------------------------------------------------------------------------
// kernel.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"

// ------------------------------------------------------------------------------------------------
// Low-memory
#define IDT_BASE                    0x1000
#define MEMORY_MAP                  0x5000
#define VGA_TEXT_BASE               0x000b8000

// ------------------------------------------------------------------------------------------------
// Interrupts
#define INTERRUPT_GATE              0x8e00
#define TRAP_GATE                   0x8f00

void idt_init();
void idt_set_handler(u8 index, u16 type, void (*handler)());

// ------------------------------------------------------------------------------------------------
// IO

static inline void outb(int port, u8 data)
{
    __asm__ volatile("outb %b0, %w1" : : "a" (data), "Nd" (port));
}

static inline u8 inb(int port)
{
    u8 data;
    __asm__ volatile("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void outw(int port, u16 data)
{
    __asm__ volatile("outw %w0, %w1" : : "a" (data), "Nd" (port));
}

static inline u16 inw(int port)
{
    u16 data;
    __asm__ volatile("inw %w1, %w0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void outl(int port, u32 data)
{
    __asm__ volatile("outl %0, %w1" : : "a" (data), "Nd" (port));
}

static inline u32 inl(int port)
{
    u32 data;
    __asm__ volatile("inl %w1, %0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void io_wait()
{
    __asm__ volatile("jmp 1f;1:jmp 1f;1:");
}
