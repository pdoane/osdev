// ------------------------------------------------------------------------------------------------
// vm.c
// ------------------------------------------------------------------------------------------------

#include "vm.h"
#include "console.h"
#include "lowmem.h"

// ------------------------------------------------------------------------------------------------
typedef struct MemoryRegion
{
    u64 start;
    u64 size;
    u32 type;
    u32 acpi_3_0;
} MemoryRegion;

// ------------------------------------------------------------------------------------------------
static void dump_memory_map()
{
    int i = 0;

    MemoryRegion* region = (MemoryRegion*)MEMORY_MAP;
    while (region->type)
    {
        console_print("region %d: start: 0x%016llx end: 0x%016llx type: %d\n", i++,
            region->start, region->start + region->size, region->type);

        ++region;
    }
}

// ------------------------------------------------------------------------------------------------
void vm_init()
{
    dump_memory_map();
}
