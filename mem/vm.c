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

static uintptr_t s_mem_next;

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
static void vm_update_page(uintptr_t addr, uint flags)
{
    // Break address into table indices
    uint pdp_index = (addr >> 30) & 0x1ff;
    uint pd_index = (addr >> 21) & 0x1ff;
    uint pt_index = (addr >> 12) & 0x1ff;

    u64* pdp_base = (u64*)VM_PDP;
    u64 pdp = pdp_base[pdp_index];
    u64* pd_base = (u64*)(pdp & ~0xfff);
    u64 pd = pd_base[pd_index];

    u64* pt_base = (u64*)(pd & ~0xfff);
    if (pd & PD_2MB)
    {
        // Switch to page table
        u64 old_base = (u64)pt_base;
        uint old_flags = pd & 0x1f;

        pt_base = vm_alloc(4096);

        for (uint i = 0; i < 512; ++i)
        {
            pt_base[i] = old_base | old_flags;
            old_base += 0x1000;
        }

        pd = (uintptr_t)pt_base | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        pd_base[pd_index] = pd;
    }

    // Update entry
    pt_base[pt_index] = addr | flags;
}

// ------------------------------------------------------------------------------------------------
void vm_init()
{
    // Enable 4GB of memory access
    uint flags = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    vm_enable_pdp(0, VM_PD + 0x0000, 0x00000000, flags);
    vm_enable_pdp(1, VM_PD + 0x1000, 0x40000000, flags);
    vm_enable_pdp(2, VM_PD + 0x2000, 0x80000000, flags);
    vm_enable_pdp(3, VM_PD + 0x3000, 0xc0000000, flags | PAGE_CACHE_DISABLE);   // TODO - call vm_map_pages

    dump_memory_map();

    s_mem_next = 0x00200000;
}

// ------------------------------------------------------------------------------------------------
void* vm_alloc(uint size)
{
    // Align to 4k for now
    return vm_alloc_align(size, 4096);
}

// ------------------------------------------------------------------------------------------------
void* vm_alloc_align(uint size, uint align)
{
    // Align memory request
    uintptr_t offset = s_mem_next & (align - 1);
    if (offset)
    {
        s_mem_next += align - offset;
    }

    void* result = (void*)s_mem_next;
    s_mem_next += size;
    return result;
}

// ------------------------------------------------------------------------------------------------
void vm_map_pages(void* addr, uint size, uint flags)
{
    flags |= PAGE_PRESENT | PAGE_WRITE | PAGE_USER;

    u8* page = (u8*)addr;
    u8* end = page + size;

    while (page < end)
    {
        vm_update_page((uintptr_t)page & ~0xfff, flags);
        page += 4096;
    }

    // Flush TLBs
    unsigned long val;
    __asm__ volatile(
        "mov %%cr3, %0\n\t"
        "mov %0,%%cr3" : "=r" (val));
}
