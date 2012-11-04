// ------------------------------------------------------------------------------------------------
// acpi/acpi.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

#define MAX_CPU_COUNT 16

extern uint g_acpiCpuCount;
extern u8 g_acpiCpuIds[MAX_CPU_COUNT];

void AcpiInit();
uint AcpiRemapIrq(uint irq);
