// ------------------------------------------------------------------------------------------------
// lowmem.h
// ------------------------------------------------------------------------------------------------

#pragma once

#ifndef TEST

#define IDT_BASE                    0x00001000
#define VM_PML4                     0x00002000
#define VM_PDP                      0x00003000
#define MEMORY_MAP                  0x00005000
#define VGA_TEXT_BASE               ((volatile u16*)0x000b8000)

#define VM_PD                       0x00010000
#define KERNEL_BASE                 0x00100000

#else

extern u16 vga_text_base[80*25];

//#define IDT_BASE                    0x1000
//#define MEMORY_MAP                  0x5000
#define VGA_TEXT_BASE               vga_text_base

#endif
