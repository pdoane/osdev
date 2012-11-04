// ------------------------------------------------------------------------------------------------
// intr/local_apic.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

extern u8 *g_localApicAddr;

void LocalApicInit();

uint LocalApicGetId();
void LocalApicSendInit(uint apic_id);
void LocalApicSendStartup(uint apic_id, uint vector);
