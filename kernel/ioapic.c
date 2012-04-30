// ------------------------------------------------------------------------------------------------
// ioapic.c
// ------------------------------------------------------------------------------------------------

#include "ioapic.h"
#include "console.h"

// ------------------------------------------------------------------------------------------------
// Globals
volatile u8* ioapic_address;

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
static void ioapic_out(volatile u8* base, u8 reg, u32 val)
{
    *(volatile u32*)(base + IOREGSEL) = reg;
    *(volatile u32*)(base + IOWIN) = val;
}

// ------------------------------------------------------------------------------------------------
static u32 ioapic_in(volatile u8* base, u8 reg)
{
    *(volatile u32*)(base + IOREGSEL) = reg;
    return *(volatile u32*)(base + IOWIN);
}

// ------------------------------------------------------------------------------------------------
void ioapic_set_entry(volatile u8* base, u8 index, u64 data)
{
    ioapic_out(base, IOREDTBL + index * 2, (u32)data);
    ioapic_out(base, IOREDTBL + index * 2 + 1, (u32)(data >> 32));
}

// ------------------------------------------------------------------------------------------------
void ioapic_init()
{
    // Get number of entries supported by the IO APIC
    u32 x = ioapic_in(ioapic_address, IOAPICVER);
    uint count = ((x >> 16) & 0xff) + 1;    // maximum redirection entry

    console_print("I/O APIC pins = %d\n", count);

    // Disable all entries
    for (uint i = 0; i < count; ++i)
    {
        ioapic_set_entry(ioapic_address, i, 1 << 16);
    }
}
