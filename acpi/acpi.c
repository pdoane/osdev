// ------------------------------------------------------------------------------------------------
// acpi/acpi.c
// ------------------------------------------------------------------------------------------------

#include "acpi/acpi.h"
#include "console/console.h"
#include "intr/ioapic.h"
#include "intr/local_apic.h"
#include "stdlib/string.h"

// ------------------------------------------------------------------------------------------------
// Globals
uint g_acpiCpuCount;
u8 g_acpiCpuIds[MAX_CPU_COUNT];

// ------------------------------------------------------------------------------------------------
typedef struct AcpiHeader
{
    u32 signature;
    u32 length;
    u8 revision;
    u8 checksum;
    u8 oem[6];
    u8 oemTableId[8];
    u32 oemRevision;
    u32 creatorId;
    u32 creatorRevision;
} PACKED AcpiHeader;

// ------------------------------------------------------------------------------------------------
typedef struct AcpiFadt
{
    AcpiHeader header;
    u32 firmwareControl;
    u32 dsdt;
    u8 reserved;
    u8 preferredPMProfile;
    u16 sciInterrupt;
    u32 smiCommandPort;
    u8 acpiEnable;
    u8 acpiDisable;
    // TODO - fill in rest of data
} PACKED AcpiFadt;

// ------------------------------------------------------------------------------------------------
typedef struct AcpiMadt
{
    AcpiHeader header;
    u32 localApicAddr;
    u32 flags;
} PACKED AcpiMadt;

// ------------------------------------------------------------------------------------------------
typedef struct ApicHeader
{
    u8 type;
    u8 length;
} PACKED ApicHeader;

// APIC structure types
#define APIC_TYPE_LOCAL_APIC            0
#define APIC_TYPE_IO_APIC               1
#define APIC_TYPE_INTERRUPT_OVERRIDE    2

// ------------------------------------------------------------------------------------------------
typedef struct ApicLocalApic
{
    ApicHeader header;
    u8 acpiProcessorId;
    u8 apicId;
    u32 flags;
} PACKED ApicLocalApic;

// ------------------------------------------------------------------------------------------------
typedef struct ApicIoApic
{
    ApicHeader header;
    u8 ioApicId;
    u8 reserved;
    u32 ioApicAddress;
    u32 globalSystemInterruptBase;
} PACKED ApicIoApic;

// ------------------------------------------------------------------------------------------------
typedef struct ApicInterruptOverride
{
    ApicHeader header;
    u8 bus;
    u8 source;
    u32 interrupt;
    u16 flags;
} PACKED ApicInterruptOverride;

// ------------------------------------------------------------------------------------------------
static AcpiMadt *s_madt;

// ------------------------------------------------------------------------------------------------
static void AcpiParseFacp(AcpiFadt *facp)
{
    if (facp->smiCommandPort)
    {
        //ConsolePrint("Enabling ACPI\n");
        //IoWrite8(facp->smiCommandPort, facp->acpiEnable);

        // TODO - wait for SCI_EN bit
    }
    else
    {
        ConsolePrint("ACPI already enabled\n");
    }
}

// ------------------------------------------------------------------------------------------------
static void AcpiParseApic(AcpiMadt *madt)
{
    s_madt = madt;

    ConsolePrint("Local APIC Address = 0x%08x\n", madt->localApicAddr);
    g_localApicAddr = (u8 *)(uintptr_t)madt->localApicAddr;

    u8 *p = (u8 *)(madt + 1);
    u8 *end = (u8 *)madt + madt->header.length;

    while (p < end)
    {
        ApicHeader *header = (ApicHeader *)p;
        u8 type = header->type;
        u8 length = header->length;

        if (type == APIC_TYPE_LOCAL_APIC)
        {
            ApicLocalApic *s = (ApicLocalApic *)p;

            ConsolePrint("Found CPU: %d %d %x\n", s->acpiProcessorId, s->apicId, s->flags);
            if (g_acpiCpuCount < MAX_CPU_COUNT)
            {
                g_acpiCpuIds[g_acpiCpuCount] = s->apicId;
                ++g_acpiCpuCount;
            }
        }
        else if (type == APIC_TYPE_IO_APIC)
        {
            ApicIoApic *s = (ApicIoApic *)p;

            ConsolePrint("Found I/O APIC: %d 0x%08x %d\n", s->ioApicId, s->ioApicAddress, s->globalSystemInterruptBase);
            g_ioApicAddr = (u8 *)(uintptr_t)s->ioApicAddress;
        }
        else if (type == APIC_TYPE_INTERRUPT_OVERRIDE)
        {
            ApicInterruptOverride *s = (ApicInterruptOverride *)p;

            ConsolePrint("Found Interrupt Override: %d %d %d 0x%04x\n", s->bus, s->source, s->interrupt, s->flags);
        }
        else
        {
            ConsolePrint("Unknown APIC structure %d\n", type);
        }

        p += length;
    }
}

// ------------------------------------------------------------------------------------------------
static void AcpiParseDT(AcpiHeader *header)
{
    u32 signature = header->signature;

    char sigStr[5];
    memcpy(sigStr, &signature, 4);
    sigStr[4] = 0;
    ConsolePrint("%s 0x%x\n", sigStr, signature);

    if (signature == 0x50434146)
    {
        AcpiParseFacp((AcpiFadt *)header);
    }
    else if (signature == 0x43495041)
    {
        AcpiParseApic((AcpiMadt *)header);
    }
}

// ------------------------------------------------------------------------------------------------
static void AcpiParseRsdt(AcpiHeader *rsdt)
{
    u32 *p = (u32 *)(rsdt + 1);
    u32 *end = (u32 *)((u8*)rsdt + rsdt->length);

    while (p < end)
    {
        u32 address = *p++;
        AcpiParseDT((AcpiHeader *)(uintptr_t)address);
    }
}

// ------------------------------------------------------------------------------------------------
static void AcpiParseXsdt(AcpiHeader *xsdt)
{
    u64 *p = (u64 *)(xsdt + 1);
    u64 *end = (u64 *)((u8*)xsdt + xsdt->length);

    while (p < end)
    {
        u64 address = *p++;
        AcpiParseDT((AcpiHeader *)(uintptr_t)address);
    }
}

// ------------------------------------------------------------------------------------------------
static bool AcpiParseRsdp(u8 *p)
{
    // Parse Root System Description Pointer
    ConsolePrint("RSDP found\n");

    // Verify checksum
    u8 sum = 0;
    for (uint i = 0; i < 20; ++i)
    {
        sum += p[i];
    }

    if (sum)
    {
        ConsolePrint("Checksum failed\n");
        return false;
    }

    // Print OEM
    char oem[7];
    memcpy(oem, p + 9, 6);
    oem[6] = '\0';
    ConsolePrint("OEM = %s\n", oem);

    // Check version
    u8 revision = p[15];
    if (revision == 0)
    {
        ConsolePrint("Version 1\n");

        u32 rsdtAddr = *(u32 *)(p + 16);
        AcpiParseRsdt((AcpiHeader *)(uintptr_t)rsdtAddr);
    }
    else if (revision == 2)
    {
        ConsolePrint("Version 2\n");

        u32 rsdtAddr = *(u32 *)(p + 16);
        u64 xsdtAddr = *(u64 *)(p + 24);

        if (xsdtAddr)
        {
            AcpiParseXsdt((AcpiHeader *)(uintptr_t)xsdtAddr);
        }
        else
        {
            AcpiParseRsdt((AcpiHeader *)(uintptr_t)rsdtAddr);
        }
    }
    else
    {
        ConsolePrint("Unsupported ACPI version %d\n", revision);
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
void AcpiInit()
{
    // TODO - Search Extended BIOS Area

    // Search main BIOS area below 1MB
    u8 *p = (u8 *)0x000e0000;
    u8 *end = (u8 *)0x000fffff;

    while (p < end)
    {
        u64 signature = *(u64 *)p;

        if (signature == 0x2052545020445352) // 'RSD PTR '
        {
            if (AcpiParseRsdp(p))
            {
                break;
            }
        }

        p += 16;
    }
}

// ------------------------------------------------------------------------------------------------
uint AcpiRemapIrq(uint irq)
{
    AcpiMadt *madt = s_madt;

    u8 *p = (u8 *)(madt + 1);
    u8 *end = (u8 *)madt + madt->header.length;

    while (p < end)
    {
        ApicHeader *header = (ApicHeader *)p;
        u8 type = header->type;
        u8 length = header->length;

        if (type == APIC_TYPE_INTERRUPT_OVERRIDE)
        {
            ApicInterruptOverride *s = (ApicInterruptOverride *)p;

            if (s->source == irq)
            {
                return s->interrupt;
            }
        }

        p += length;
    }

    return irq;
}
