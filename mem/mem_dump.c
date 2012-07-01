// ------------------------------------------------------------------------------------------------
// mem/mem_dump.c
// ------------------------------------------------------------------------------------------------

#include "mem/mem_dump.h"
#include "console/console.h"

// ------------------------------------------------------------------------------------------------
static uint mem_char(uint val)
{
    return (val >= 0x20 && val < 0x80) ? val : '.';
}

// ------------------------------------------------------------------------------------------------
void mem_dump(const void* start, const void* end)
{
    console_print("Memdump from 0x%016x to 0x%016x\n", start, end);

    u8* p = (u8*)start;

    while (p < (u8*)end)
    {
        uint offset = (u64)p & 0xffff;
        console_print("%04x:  "
                "%02x %02x %02x %02x  "
                "%02x %02x %02x %02x  "
                "%02x %02x %02x %02x  "
                "%02x %02x %02x %02x  "
                "%c%c%c%c%c%c%c%c"
                "%c%c%c%c%c%c%c%c\n",
                offset,
                p[0x0], p[0x1], p[0x2], p[0x3],
                p[0x4], p[0x5], p[0x6], p[0x7],
                p[0x8], p[0x9], p[0xa], p[0xb],
                p[0xc], p[0xd], p[0xe], p[0xf],
                mem_char(p[0x0]), mem_char(p[0x1]),
                mem_char(p[0x2]), mem_char(p[0x3]),
                mem_char(p[0x4]), mem_char(p[0x5]),
                mem_char(p[0x6]), mem_char(p[0x7]),
                mem_char(p[0x8]), mem_char(p[0x9]),
                mem_char(p[0xa]), mem_char(p[0xb]),
                mem_char(p[0xc]), mem_char(p[0xd]),
                mem_char(p[0xe]), mem_char(p[0xf]));

        p += 16;
    }
}

