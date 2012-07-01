// ------------------------------------------------------------------------------------------------
// input/keymap.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

typedef struct Keymap
{
    u8 base[128];
    u8 shift[128];
    u8 numlock[128];
} Keymap;

extern Keymap keymap_us101;
