// ------------------------------------------------------------------------------------------------
// io.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"

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
