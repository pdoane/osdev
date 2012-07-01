// ------------------------------------------------------------------------------------------------
// mem/vm.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

void vm_init();

void* vm_alloc(uint size);
void* vm_alloc_align(uint size, uint align);
