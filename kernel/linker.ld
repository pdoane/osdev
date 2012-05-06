OUTPUT_FORMAT("binary")
ENTRY(kmain)
SECTIONS
{
    local_apic_address = 0x00006000;
    active_cpu_count = 0x00006008;

    .text 0x0000000000100000:
    {
        *(.text)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        __bss_start = .;
        *(.bss)
        __bss_end = .;
    }
}
