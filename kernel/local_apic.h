// ------------------------------------------------------------------------------------------------
// local_apic.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"

extern u8* local_apic_address;

void lapic_init();

uint lapic_getid();
void lapic_send_init(uint apic_id);
void lapic_send_startup(uint apic_id, uint vector);
