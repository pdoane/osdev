// ------------------------------------------------------------------------------------------------
// mem/vm.c
// ------------------------------------------------------------------------------------------------

#include "mem/vm.h"
#include "mem/lowmem.h"
#include "console/console.h"

#define PAGE_PRESENT                    0x01    // Must be 1 to reference page-directory
#define PAGE_WRITE                      0x02    // Write access
#define PAGE_USER                       0x04    // User-mode access
#define PAGE_WRITE_THROUGH              0x08    // Page-level write-through
#define PAGE_CACHE_DISABLE              0x10    // Page-level cache-disable

#define PD_2MB                          0x80    // 2MB Page

static u64 s_mem_next;

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
static void vm_enable_pd(u64 pd_offset, u64 base, uint flags)
{
    u64* pd = (u64*)pd_offset;
    for (uint i = 0; i < 512; ++i)
    {
        pd[i] = base | PD_2MB | flags;
        base += 0x200000;
    }
}

// ------------------------------------------------------------------------------------------------
static void vm_enable_pdp(uint pdp_index, u64 pd_offset, u64 base, uint flags)
{
    vm_enable_pd(pd_offset, base, flags);
    u64* pdp = (u64*)VM_PDP;
    pdp[pdp_index] = pd_offset | flags;
}

// ------------------------------------------------------------------------------------------------
void vm_init()
{
    // Enable 4GB of memory access
    uint flags = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    vm_enable_pdp(0, VM_PD + 0x0000, 0x00000000, flags);
    vm_enable_pdp(1, VM_PD + 0x1000, 0x40000000, flags);
    vm_enable_pdp(2, VM_PD + 0x2000, 0x80000000, flags);
    vm_enable_pdp(3, VM_PD + 0x3000, 0xc0000000, flags | PAGE_CACHE_DISABLE);

    dump_memory_map();

    s_mem_next = 0x00200000;
}

// ------------------------------------------------------------------------------------------------
void* vm_alloc(uint size)
{
    // Round all requests up to 4k page size for now
    size = (size + 4095) & ~4095;
    void* result = (void*)s_mem_next;
    s_mem_next += size;
    return result;
}
