// ------------------------------------------------------------------------------------------------
// cpu/io.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
// x86 in/out instructions

static inline void out8(uint port, u8 data)
{
    __asm__ volatile("outb %b0, %w1" : : "a" (data), "Nd" (port));
}

static inline u8 in8(uint port)
{
    u8 data;
    __asm__ volatile("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void out16(uint port, u16 data)
{
    __asm__ volatile("outw %w0, %w1" : : "a" (data), "Nd" (port));
}

static inline u16 in16(uint port)
{
    u16 data;
    __asm__ volatile("inw %w1, %w0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void out32(uint port, u32 data)
{
    __asm__ volatile("outl %0, %w1" : : "a" (data), "Nd" (port));
}

static inline u32 in32(uint port)
{
    u32 data;
    __asm__ volatile("inl %w1, %0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void io_wait()
{
    __asm__ volatile("jmp 1f;1:jmp 1f;1:");
}

// ------------------------------------------------------------------------------------------------
// memory mapped i/o functions

static inline void mmio_write8(void* p, u8 data)
{
    *(volatile u8*)(p) = data;
}

static inline u8 mmio_read8(void* p)
{
    return *(volatile u8*)(p);
}

static inline void mmio_write16(void* p, u16 data)
{
    *(volatile u16*)(p) = data;
}

static inline u16 mmio_read16(void* p)
{
    return *(volatile u16*)(p);
}

static inline void mmio_write32(void* p, u32 data)
{
    *(volatile u32*)(p) = data;
}

static inline u32 mmio_read32(void* p)
{
    return *(volatile u32*)(p);
}

static inline void mmio_write64(void* p, u64 data)
{
    *(volatile u64*)(p) = data;
}

static inline u64 mmio_read64(void* p)
{
    return *(volatile u64*)(p);
}

static inline void mmio_readN(void *dst, const volatile void *src, size_t bytes)
{
    volatile u8 *s = (volatile u8 *)src;
    u8 *d = (u8 *)dst;
    while (bytes > 0)
    {
        *d =  *s;
        ++s;
        ++d;
        --bytes;
    }
}
