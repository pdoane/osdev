// ------------------------------------------------------------------------------------------------
// acpi/acpi.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

#define MAX_CPU_COUNT 16

extern uint acpi_cpu_count;
extern u8 acpi_cpu_ids[MAX_CPU_COUNT];

void acpi_init();
uint acpi_remap_irq(uint irq);
