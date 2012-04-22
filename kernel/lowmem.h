// ------------------------------------------------------------------------------------------------
// lowmem.h
// ------------------------------------------------------------------------------------------------

#pragma once

#ifndef TEST

#define IDT_BASE                    0x1000
#define MEMORY_MAP                  0x5000
#define VGA_TEXT_BASE               ((volatile u16*)0xb8000)

#else

extern u16 vga_text_base[80*25];

//#define IDT_BASE                    0x1000
//#define MEMORY_MAP                  0x5000
#define VGA_TEXT_BASE               vga_text_base

#endif
