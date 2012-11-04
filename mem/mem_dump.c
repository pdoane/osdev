// ------------------------------------------------------------------------------------------------
// mem/mem_dump.c
// ------------------------------------------------------------------------------------------------

#include "mem/mem_dump.h"
#include "console/console.h"

// ------------------------------------------------------------------------------------------------
static uint MemChar(uint val)
{
    return (val >= 0x20 && val < 0x80) ? val : '.';
}

// ------------------------------------------------------------------------------------------------
void MemDump(const void *start, const void *end)
{
    ConsolePrint("Memdump from 0x%016x to 0x%016x\n", start, end);

    u8 *p = (u8 *)start;

    while (p < (u8 *)end)
    {
        uint offset = (u64)p & 0xffff;
        ConsolePrint("%04x:  "
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
                MemChar(p[0x0]), MemChar(p[0x1]),
                MemChar(p[0x2]), MemChar(p[0x3]),
                MemChar(p[0x4]), MemChar(p[0x5]),
                MemChar(p[0x6]), MemChar(p[0x7]),
                MemChar(p[0x8]), MemChar(p[0x9]),
                MemChar(p[0xa]), MemChar(p[0xb]),
                MemChar(p[0xc]), MemChar(p[0xd]),
                MemChar(p[0xe]), MemChar(p[0xf]));

        p += 16;
    }
}

