// ------------------------------------------------------------------------------------------------
// smp.c
// ------------------------------------------------------------------------------------------------

#include "smp.h"
#include "acpi.h"
#include "console.h"
#include "local_apic.h"
#include "pit.h"

// ------------------------------------------------------------------------------------------------
void smp_init()
{
    console_print("Waking up all CPUs\n");

    active_cpu_count = 1;
    uint local_id = lapic_getid();

    // Send Init to all cpus except self
    for (uint i = 0; i < acpi_cpu_count; ++i)
    {
        uint apic_id = acpi_cpu_ids[i];
        if (apic_id != local_id)
        {
            lapic_send_init(apic_id);
        }
    }

    // wait
    pit_wait(10);

    // Send Startup to all cpus except self
    for (uint i = 0; i < acpi_cpu_count; ++i)
    {
        uint apic_id = acpi_cpu_ids[i];
        if (apic_id != local_id)
        {
            lapic_send_startup(apic_id, 0x8);
        }
    }

    // Wait for all cpus to be active
    pit_wait(1);
    while (active_cpu_count != acpi_cpu_count)
    {
        console_print("Waiting... %d\n", active_cpu_count);
        pit_wait(1);
    }

    console_print("All CPUs activated\n");
}
