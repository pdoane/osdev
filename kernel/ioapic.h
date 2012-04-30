// ------------------------------------------------------------------------------------------------
// ioapic.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"

extern volatile u8* ioapic_address;

void ioapic_init();
void ioapic_set_entry(volatile u8* base, u8 index, u64 data);
