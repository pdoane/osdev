// ------------------------------------------------------------------------------------------------
// kernel.c
// ------------------------------------------------------------------------------------------------

#include "format.h"

#define VIDMEM_BASE 0xb8000

typedef struct MemoryRegion
{
    u64 start;
    u64 size;
    u32 type;
    u32 acpi_3_0;
} MemoryRegion;

// ------------------------------------------------------------------------------------------------
static void clear()
{
    volatile u64* p = (volatile u64*)VIDMEM_BASE;
    for (int i = 0; i < 500; ++i)
    {
        *p++ = 0x1f201f201f201f20;
    }
}

// ------------------------------------------------------------------------------------------------
static void print(const char* s, uint line)
{
    volatile u8* p = (volatile u8*)VIDMEM_BASE;

    p += line * 80 * 2;

    while (*s)
    {
        *p++ = *s++;
        *p++ = 0x1f;
    }
}

// ------------------------------------------------------------------------------------------------
int kmain()
{
    char buf[80];

    clear();

    uint line = 0;
    MemoryRegion* region = (MemoryRegion*)0x1100;
    while (region->type)
    {
        snprintf(buf, sizeof(buf), "region %d: start: 0x%016llx end: 0x%016llx type: %d", line,
            region->start, region->start + region->size, region->type);
        print(buf, line);

        ++region;
        ++line;
    }

    for (;;)
    {
    }

    return 0;
}
