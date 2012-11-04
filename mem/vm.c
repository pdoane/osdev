// ------------------------------------------------------------------------------------------------
// mem/vm.c
// ------------------------------------------------------------------------------------------------

#include "mem/vm.h"
#include "mem/lowmem.h"
#include "console/console.h"

// ------------------------------------------------------------------------------------------------
#define PAGE_PRESENT                    0x01    // Must be 1 to reference page-directory
#define PAGE_WRITE                      0x02    // Write access
#define PAGE_USER                       0x04    // User-mode access
#define PAGE_WRITE_THROUGH              0x08    // Page-level write-through
#define PAGE_CACHE_DISABLE              0x10    // Page-level cache-disable

#define PD_2MB                          0x80    // 2MB Page

static uintptr_t s_nextAlloc;

// ------------------------------------------------------------------------------------------------
typedef struct MemoryRegion
{
    u64 start;
    u64 size;
    u32 type;
    u32 acpi_3_0;
} MemoryRegion;

// ------------------------------------------------------------------------------------------------
static void DumpMemoryMap()
{
    int i = 0;

    MemoryRegion *region = (MemoryRegion *)MEMORY_MAP;
    while (region->type)
    {
        ConsolePrint("region %d: start: 0x%016llx end: 0x%016llx type: %d\n", i++,
            region->start, region->start + region->size, region->type);

        ++region;
    }
}

// ------------------------------------------------------------------------------------------------
static void VMEnablePD(u64 pdOffset, u64 base, uint flags)
{
    u64 *pd = (u64 *)pdOffset;
    for (uint i = 0; i < 512; ++i)
    {
        pd[i] = base | PD_2MB | flags;
        base += 0x200000;
    }
}

// ------------------------------------------------------------------------------------------------
static void VMEnablePDP(uint pdpIndex, u64 pdOffset, u64 base, uint flags)
{
    VMEnablePD(pdOffset, base, flags);
    u64 *pdp = (u64 *)VM_PDP;
    pdp[pdpIndex] = pdOffset | flags;
}

// ------------------------------------------------------------------------------------------------
static void VMUpdatePage(uintptr_t addr, uint flags)
{
    // Break address into table indices
    uint pdpIndex = (addr >> 30) & 0x1ff;
    uint pdIndex = (addr >> 21) & 0x1ff;
    uint ptIndex = (addr >> 12) & 0x1ff;

    u64 *pdpBase = (u64 *)VM_PDP;
    u64 pdp = pdpBase[pdpIndex];
    u64 *pdBase = (u64 *)(pdp & ~0xfff);
    u64 pd = pdBase[pdIndex];

    u64 *ptBase = (u64 *)(pd & ~0xfff);
    if (pd & PD_2MB)
    {
        // Switch to page table
        u64 oldBase = (u64)ptBase;
        uint oldFlags = pd & 0x1f;

        ptBase = VMAlloc(4096);

        for (uint i = 0; i < 512; ++i)
        {
            ptBase[i] = oldBase | oldFlags;
            oldBase += 0x1000;
        }

        pd = (uintptr_t)ptBase | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        pdBase[pdIndex] = pd;
    }

    // Update entry
    ptBase[ptIndex] = addr | flags;
}

// ------------------------------------------------------------------------------------------------
void VMInit()
{
    // Enable 4GB of memory access
    uint flags = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    VMEnablePDP(0, VM_PD + 0x0000, 0x00000000, flags);
    VMEnablePDP(1, VM_PD + 0x1000, 0x40000000, flags);
    VMEnablePDP(2, VM_PD + 0x2000, 0x80000000, flags);
    VMEnablePDP(3, VM_PD + 0x3000, 0xc0000000, flags | PAGE_CACHE_DISABLE);   // TODO - call VMMapPages

    DumpMemoryMap();

    s_nextAlloc = 0x00200000;
}

// ------------------------------------------------------------------------------------------------
void *VMAlloc(uint size)
{
    // Align to 4k for now
    return VMAllocAlign(size, 4096);
}

// ------------------------------------------------------------------------------------------------
void *VMAllocAlign(uint size, uint align)
{
    // Align memory request
    uintptr_t offset = s_nextAlloc & (align - 1);
    if (offset)
    {
        s_nextAlloc += align - offset;
    }

    void *result = (void *)s_nextAlloc;
    s_nextAlloc += size;
    return result;
}

// ------------------------------------------------------------------------------------------------
void VMMapPages(void *addr, uint size, uint flags)
{
    flags |= PAGE_PRESENT | PAGE_WRITE | PAGE_USER;

    u8 *page = (u8 *)addr;
    u8 *end = page + size;

    while (page < end)
    {
        VMUpdatePage((uintptr_t)page & ~0xfff, flags);
        page += 4096;
    }

    // Flush TLBs
    unsigned long val;
    __asm__ volatile(
        "mov %%cr3, %0\n\t"
        "mov %0,%%cr3" : "=r" (val));
}
