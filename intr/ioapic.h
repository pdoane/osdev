// ------------------------------------------------------------------------------------------------
// intr/ioapic.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

extern u8* ioapic_address;

void ioapic_init();
void ioapic_set_entry(u8* base, u8 index, u64 data);
