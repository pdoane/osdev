// ------------------------------------------------------------------------------------------------
// keyboard.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"

#define KBD_LSHIFT                      0x01
#define KBD_RSHIFT                      0x02
#define KBD_CAPS_LOCK                   0x04
#define KBD_NUM_LOCK                    0x08
#define KBD_ESCAPE_SEQUENCE             0x10

void keyboard_poll();

u8 keyboard_get_flags();
