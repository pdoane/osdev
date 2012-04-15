OUTPUT_FORMAT("binary")
ENTRY(kmain)
SECTIONS
{
    .text 0x0000000000100000 :
    {
        *(.text)
    }
    .data :
    {
        *(.data)
    }
    .bss :
    {
        *(.bss)
    }
}
