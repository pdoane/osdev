// ------------------------------------------------------------------------------------------------
// cpu/io.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
// x86 in/out instructions

static inline void IoWrite8(uint port, u8 data)
{
    __asm__ volatile("outb %b0, %w1" : : "a" (data), "Nd" (port));
}

static inline u8 IoRead8(uint port)
{
    u8 data;
    __asm__ volatile("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void IoWrite16(uint port, u16 data)
{
    __asm__ volatile("outw %w0, %w1" : : "a" (data), "Nd" (port));
}

static inline u16 IoRead16(uint port)
{
    u16 data;
    __asm__ volatile("inw %w1, %w0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void IoWrite32(uint port, u32 data)
{
    __asm__ volatile("outl %0, %w1" : : "a" (data), "Nd" (port));
}

static inline u32 IoRead32(uint port)
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

static inline void MmioWrite8(void *p, u8 data)
{
    *(volatile u8 *)(p) = data;
}

static inline u8 MmioRead8(void *p)
{
    return *(volatile u8 *)(p);
}

static inline void MmioWrite16(void *p, u16 data)
{
    *(volatile u16 *)(p) = data;
}

static inline u16 MmioRead16(void *p)
{
    return *(volatile u16 *)(p);
}

static inline void MmioWrite32(void *p, u32 data)
{
    *(volatile u32 *)(p) = data;
}

static inline u32 MmioRead32(void *p)
{
    return *(volatile u32 *)(p);
}

static inline void MmioWrite64(void *p, u64 data)
{
    *(volatile u64 *)(p) = data;
}

static inline u64 MmioRead64(void *p)
{
    return *(volatile u64 *)(p);
}

static inline void MmioReadN(void *dst, const volatile void *src, size_t bytes)
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
