// ------------------------------------------------------------------------------------------------
// acpi.c
// ------------------------------------------------------------------------------------------------

#include "acpi.h"
#include "console.h"
#include "ioapic.h"
#include "local_apic.h"
#include "string.h"

// ------------------------------------------------------------------------------------------------
typedef struct ACPI_Header
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
} PACKED ACPI_Header;

// ------------------------------------------------------------------------------------------------
typedef struct ACPI_FADT
{
    ACPI_Header header;
    u32 firmwareControl;
    u32 dsdt;
    u8 reserved;
    u8 preferredPMProfile;
    u16 sciInterrupt;
    u32 smiCommandPort;
    u8 acpiEnable;
    u8 acpiDisable;
    // TODO - fill in rest of data
} PACKED ACPI_FADT;

// ------------------------------------------------------------------------------------------------
typedef struct ACPI_MADT
{
    ACPI_Header header;
    u32 localApicAddr;
    u32 flags;
} PACKED ACPI_MADT;

// ------------------------------------------------------------------------------------------------
typedef struct APIC_Header
{
    u8 type;
    u8 length;
} PACKED APIC_Header;

// ------------------------------------------------------------------------------------------------
typedef struct ACPI_ProcessorLocalAPIC
{
    APIC_Header header;
    u8 acpiProcessorId;
    u8 apicId;
    u32 flags;
} PACKED ACPI_ProcessorLocalAPIC;

// ------------------------------------------------------------------------------------------------
typedef struct ACPI_IO_APIC
{
    APIC_Header header;
    u8 ioApicId;
    u8 reserved;
    u32 ioApicAddress;
    u32 globalSystemInterruptBase;
} PACKED ACPI_IO_APIC;

// ------------------------------------------------------------------------------------------------
typedef struct ACPI_InterruptSourceOverride
{
    APIC_Header header;
    u8 bus;
    u8 source;
    u32 globalSystemInterrupt;
    u16 flags;
} PACKED ACPI_InterruptSourceOverride;

// ------------------------------------------------------------------------------------------------
static void acpi_parse_facp(ACPI_FADT* facp)
{
    if (facp->smiCommandPort)
    {
        //console_print("Enabling ACPI\n");
        //outb(facp->smiCommandPort, facp->acpiEnable);

        // TODO - wait for SCI_EN bit
    }
    else
    {
        console_print("ACPI already enabled\n");
    }
}

// ------------------------------------------------------------------------------------------------
static void acpi_parse_apic(ACPI_MADT* madt)
{
    console_print("Local APIC Address = 0x%08x\n", madt->localApicAddr);
    local_apic_address = (u8*)(uintptr_t)madt->localApicAddr;

    u8* p = (u8*)(madt + 1);
    u8* end = (u8*)madt + madt->header.length;

    while (p < end)
    {
        u8 type = *(p + 0);
        u8 length = *(p + 1);

        if (type == 0)
        {
            ACPI_ProcessorLocalAPIC* s = (ACPI_ProcessorLocalAPIC*)p;

            console_print("Found CPU: %d %d %x\n", s->acpiProcessorId, s->apicId, s->flags);
        }
        else if (type == 1)
        {
            ACPI_IO_APIC* s = (ACPI_IO_APIC*)p;

            console_print("Found I/O APIC: %d 0x%08x %d\n", s->ioApicId, s->ioApicAddress, s->globalSystemInterruptBase);
            ioapic_address = (u8*)(uintptr_t)s->ioApicAddress;
        }
        else if (type == 2)
        {
            ACPI_InterruptSourceOverride* s = (ACPI_InterruptSourceOverride*)p;

            console_print("Found Interrupt Override: %d %d %d 0x%04x\n", s->bus, s->source, s->globalSystemInterrupt, s->flags);
        }
        else
        {
            console_print("Unknown APIC structure %d\n", type);
        }

        p += length;
    }
}

// ------------------------------------------------------------------------------------------------
static void acpi_parse_dt(ACPI_Header* header)
{
    u32 signature = header->signature;

    char sigStr[5];
    memcpy(sigStr, &signature, 4);
    sigStr[4] = 0;
    console_print("%s 0x%x\n", sigStr, signature);

    if (signature == 0x50434146)
    {
        acpi_parse_facp((ACPI_FADT*)header);
    }
    else if (signature == 0x43495041)
    {
        acpi_parse_apic((ACPI_MADT*)header);
    }
}

// ------------------------------------------------------------------------------------------------
static void acpi_parse_rsdt(ACPI_Header* rsdt)
{
    u32* p = (u32*)(rsdt + 1);
    u32* end = (u32*)((u8*)rsdt + rsdt->length);

    while (p < end)
    {
        u32 address = *p++;
        acpi_parse_dt((ACPI_Header*)(uintptr_t)address);
    }
}

// ------------------------------------------------------------------------------------------------
static void acpi_parse_xsdt(ACPI_Header* xsdt)
{
    u64* p = (u64*)(xsdt + 1);
    u64* end = (u64*)((u8*)xsdt + xsdt->length);

    while (p < end)
    {
        u64 address = *p++;
        acpi_parse_dt((ACPI_Header*)(uintptr_t)address);
    }
}

// ------------------------------------------------------------------------------------------------
static bool acpi_parse_rsdp(u8* p)
{
    // Parse Root System Description Pointer
    console_print("RSDP found\n");

    // Verify checksum
    u8 sum = 0;
    for (uint i = 0; i < 20; ++i)
    {
        sum += p[i];
    }

    if (sum)
    {
        console_print("Checksum failed\n");
        return false;
    }

    // Print OEM
    char oem[7];
    memcpy(oem, p + 9, 6);
    oem[6] = '\0';
    console_print("OEM = %s\n", oem);

    // Check version
    u8 revision = p[15];
    if (revision == 0)
    {
        console_print("Version 1\n");

        u32 rsdtAddr = *(u32*)(p + 16);
        acpi_parse_rsdt((ACPI_Header*)(uintptr_t)rsdtAddr);
    }
    else if (revision == 2)
    {
        console_print("Version 2\n");

        u32 rsdtAddr = *(u32*)(p + 16);
        u64 xsdtAddr = *(u64*)(p + 24);

        if (xsdtAddr)
        {
            acpi_parse_xsdt((ACPI_Header*)(uintptr_t)xsdtAddr);
        }
        else
        {
            acpi_parse_rsdt((ACPI_Header*)(uintptr_t)rsdtAddr);
        }
    }
    else
    {
        console_print("Unsupported ACPI version %d\n", revision);
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
void acpi_init()
{
    // TODO - Search Extended BIOS Area

    // Search main BIOS area below 1MB
    u8* p = (u8*)0x000e0000;
    u8* end = (u8*)0x000fffff;

    while (p < end)
    {
        u64 signature = *(u64*)p;

        if (signature == 0x2052545020445352) // 'RSD PTR '
        {
            if (acpi_parse_rsdp(p))
            {
                break;
            }
        }

        p += 16;
    }
}

