// ------------------------------------------------------------------------------------------------
// console.c
// ------------------------------------------------------------------------------------------------

#include "console.h"
#include "format.h"
#include "lowmem.h"
#include "vga.h"

// ------------------------------------------------------------------------------------------------
static uint s_col;
static uint s_row;

// ------------------------------------------------------------------------------------------------
void console_init()
{
    s_col = 0;
    s_row = 0;
}

// ------------------------------------------------------------------------------------------------
void console_putchar(char c)
{
    if (c == '\n')
    {
        // Advance to next line
        s_col = 0;
        ++s_row;
    }

    uint offset = s_row * TEXT_COLS + s_col;
    if (offset >= TEXT_ROWS * TEXT_COLS)
    {
        // Scroll text
        uint i = 0;
        for (; i < (TEXT_ROWS-1) * TEXT_COLS; i++)
        {
            VGA_TEXT_BASE[i] = VGA_TEXT_BASE[TEXT_COLS + i];
        }

        for (; i < TEXT_ROWS * TEXT_COLS; i++)
        {
            VGA_TEXT_BASE[i] = DEFAULT_TEXT_ATTR | ' ';
        }

        --s_row;
        offset -= TEXT_COLS;
    }

    if (s_col < 80 && c >= 32 && c <= 126)
    {
        // Print character
        VGA_TEXT_BASE[offset] = DEFAULT_TEXT_ATTR | c;
        ++s_col;
    }
}

// ------------------------------------------------------------------------------------------------
void console_print(const char* fmt, ...)
{
    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    char c;
    char* s = buf;
    while ((c = *s++))
    {
        console_putchar(c);
    }

    uint offset = s_row * TEXT_COLS + s_col;
    vga_text_setcursor(offset);
}
