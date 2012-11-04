// ------------------------------------------------------------------------------------------------
// cpu/smp.c
// ------------------------------------------------------------------------------------------------

#include "cpu/smp.h"
#include "acpi/acpi.h"
#include "console/console.h"
#include "intr/local_apic.h"
#include "time/pit.h"

// ------------------------------------------------------------------------------------------------
void SmpInit()
{
    ConsolePrint("Waking up all CPUs\n");

    g_activeCpuCount = 1;
    uint localId = LocalApicGetId();

    // Send Init to all cpus except self
    for (uint i = 0; i < g_acpiCpuCount; ++i)
    {
        uint apicId = g_acpiCpuIds[i];
        if (apicId != localId)
        {
            LocalApicSendInit(apicId);
        }
    }

    // wait
    PitWait(10);

    // Send Startup to all cpus except self
    for (uint i = 0; i < g_acpiCpuCount; ++i)
    {
        uint apicId = g_acpiCpuIds[i];
        if (apicId != localId)
        {
            LocalApicSendStartup(apicId, 0x8);
        }
    }

    // Wait for all cpus to be active
    PitWait(1);
    while (g_activeCpuCount != g_acpiCpuCount)
    {
        ConsolePrint("Waiting... %d\n", g_activeCpuCount);
        PitWait(1);
    }

    ConsolePrint("All CPUs activated\n");
}
