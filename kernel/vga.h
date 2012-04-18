// ------------------------------------------------------------------------------------------------
// vga.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"

// ------------------------------------------------------------------------------------------------
// VGA Text Mode
#define TEXT_ROWS                       25
#define TEXT_COLS                       80

// Colors
#define TEXT_BLACK                      0
#define TEXT_BLUE                       1
#define TEXT_GREEN                      2
#define TEXT_CYAN                       3
#define TEXT_RED                        4
#define TEXT_MAGENTA                    5
#define TEXT_BROWN                      6
#define TEXT_LIGHT_GRAY                 7
#define TEXT_DARK_GRAY                  8
#define TEXT_LIGHT_BLUE                 9
#define TEXT_LIGHT_GREEN                10
#define TEXT_LIGHT_CYAN                 11
#define TEXT_LIGHT_RED                  12
#define TEXT_LIGHT_MAGENTA              13
#define TEXT_YELLOW                     14
#define TEXT_WHITE                      15

// Text attribute
#define TEXT_ATTR(fgcolor, bgcolor) \
    ((bgcolor << 12) | (fgcolor << 8))

// Light gray text on a black background
#define DEFAULT_TEXT_ATTR               TEXT_ATTR(TEXT_LIGHT_GRAY, TEXT_BLUE)

// ------------------------------------------------------------------------------------------------
void vga_text_init();
void vga_text_clear();
void vga_text_setcursor(uint offset);
