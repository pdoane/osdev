// ------------------------------------------------------------------------------------------------
// idt.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"

#define INTERRUPT_GATE              0x8e00
#define TRAP_GATE                   0x8f00

void idt_init();
void idt_set_handler(u8 index, u16 type, void (*handler)());
