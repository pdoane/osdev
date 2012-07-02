// ------------------------------------------------------------------------------------------------
// mem/vm.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
// Page Flags

#define PAGE_WRITE_THROUGH              0x08    // Page-level write-through
#define PAGE_CACHE_DISABLE              0x10    // Page-level cache-disable

// ------------------------------------------------------------------------------------------------
void vm_init();

void* vm_alloc(uint size);
void* vm_alloc_align(uint size, uint align);

void vm_map_pages(void* addr, uint size, uint flags);
