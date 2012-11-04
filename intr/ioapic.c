// ------------------------------------------------------------------------------------------------
// intr/ioapic.c
// ------------------------------------------------------------------------------------------------

#include "intr/ioapic.h"
#include "console/console.h"
#include "cpu/io.h"

// ------------------------------------------------------------------------------------------------
// Globals
u8 *g_ioApicAddr;

// ------------------------------------------------------------------------------------------------
// Memory mapped registers for IO APIC register access
#define IOREGSEL                        0x00
#define IOWIN                           0x10

// ------------------------------------------------------------------------------------------------
// IO APIC Registers
#define IOAPICID                        0x00
#define IOAPICVER                       0x01
#define IOAPICARB                       0x02
#define IOREDTBL                        0x10

// ------------------------------------------------------------------------------------------------
static void IoApicOut(u8 *base, u8 reg, u32 val)
{
    MmioWrite32(base + IOREGSEL, reg);
    MmioWrite32(base + IOWIN, val);
}

// ------------------------------------------------------------------------------------------------
static u32 IoApicIn(u8 *base, u8 reg)
{
    MmioWrite32(base + IOREGSEL, reg);
    return MmioRead32(base + IOWIN);
}

// ------------------------------------------------------------------------------------------------
void IoApicSetEntry(u8 *base, u8 index, u64 data)
{
    IoApicOut(base, IOREDTBL + index * 2, (u32)data);
    IoApicOut(base, IOREDTBL + index * 2 + 1, (u32)(data >> 32));
}

// ------------------------------------------------------------------------------------------------
void IoApicInit()
{
    // Get number of entries supported by the IO APIC
    u32 x = IoApicIn(g_ioApicAddr, IOAPICVER);
    uint count = ((x >> 16) & 0xff) + 1;    // maximum redirection entry

    ConsolePrint("I/O APIC pins = %d\n", count);

    // Disable all entries
    for (uint i = 0; i < count; ++i)
    {
        IoApicSetEntry(g_ioApicAddr, i, 1 << 16);
    }
}
