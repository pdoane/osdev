// ------------------------------------------------------------------------------------------------
// intr/intr.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

#define IRQ_BASE                        0x20

#define IRQ_TIMER                       0x00
#define IRQ_KEYBOARD                    0x01
#define IRQ_COM2                        0x03
#define IRQ_COM1                        0x04
#define IRQ_FLOPPY                      0x06
#define IRQ_ATA0                        0x0e
#define IRQ_ATA1                        0x0f

#define INT_TIMER                       0x20
#define INT_SPURIOUS                    0xff

// ------------------------------------------------------------------------------------------------
void intr_init();