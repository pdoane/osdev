// ------------------------------------------------------------------------------------------------
// console.c
// ------------------------------------------------------------------------------------------------

#include "console.h"
#include "format.h"
#include "keycode.h"
#include "lowmem.h"
#include "string.h"
#include "vga.h"

// ------------------------------------------------------------------------------------------------
static uint s_col;
static uint s_row;

static uint s_cursor;
static char s_inputLine[TEXT_COLS];

// ------------------------------------------------------------------------------------------------
static void console_update_input()
{
    u16 attr = TEXT_ATTR(TEXT_WHITE, TEXT_BLUE);
    char* line = console_get_input_line();

    uint offset = (TEXT_ROWS-1) * TEXT_COLS;
    vga_text_setcursor(offset + s_cursor + 2);

    VGA_TEXT_BASE[offset++] = attr | '$';
    VGA_TEXT_BASE[offset++] = attr | ' ';

    char c;
    char* s = line;
    while ((c = *s++))
    {
        VGA_TEXT_BASE[offset++] = attr | c;
    }

    while (offset < TEXT_ROWS * TEXT_COLS)
    {
        VGA_TEXT_BASE[offset++] = attr | ' ';
    }
}

// ------------------------------------------------------------------------------------------------
void console_init()
{
    console_update_input();
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
    if (offset >= (TEXT_ROWS-1) * TEXT_COLS)
    {
        // Scroll text
        uint i = 0;
        for (; i < (TEXT_ROWS-2) * TEXT_COLS; i++)
        {
            VGA_TEXT_BASE[i] = VGA_TEXT_BASE[TEXT_COLS + i];
        }

        for (; i < (TEXT_ROWS-1) * TEXT_COLS; i++)
        {
            VGA_TEXT_BASE[i] = DEFAULT_TEXT_ATTR | ' ';
        }

        --s_row;
        offset -= TEXT_COLS;
    }

    if (s_col < TEXT_COLS && c >= 32 && c <= 126)
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
}

// ------------------------------------------------------------------------------------------------
uint console_get_cursor()
{
    return s_cursor;
}

// ------------------------------------------------------------------------------------------------
char* console_get_input_line()
{
    return s_inputLine;
}

// ------------------------------------------------------------------------------------------------
void console_on_keydown(uint code)
{
    char* line;
    uint len;

    switch (code)
    {
    case KEY_BACKSPACE:
        line = console_get_input_line();
        if (s_cursor > 0)
        {
            s_cursor--;
            len = strlen(line);
            memmove(line + s_cursor, line + s_cursor + 1, len - s_cursor);
            line[len-1] = '\0';
        }
        break;

    case KEY_KP_DEC:
    case KEY_DELETE:
        line = console_get_input_line();
        len = strlen(line);
        if (len > s_cursor)
        {
            memmove(line + s_cursor, line + s_cursor + 1, len - s_cursor);
            line[len-1] = '\0';
        }
        break;

    case KEY_ENTER:
    case KEY_RETURN:
        line = console_get_input_line();
        console_print("\n$ %s\n", line);

        s_inputLine[0] = '\0';
        s_cursor = 0;
        break;

    case KEY_UP:
        break;

    case KEY_DOWN:
        break;

    case KEY_KP4:
    case KEY_LEFT:
        if (s_cursor > 0)
        {
            s_cursor --;
        }
        break;

    case KEY_KP6:
    case KEY_RIGHT:
        line = console_get_input_line();
        len = strlen(line);
        if (s_cursor < len)
        {
            s_cursor ++;
        }
        break;

    case KEY_KP7:
    case KEY_HOME:
        s_cursor = 0;
        break;

    case KEY_KP1:
    case KEY_END:
        line = console_get_input_line();
        len = strlen(line);
        s_cursor = len;
        break;
    }

    console_update_input();
}

// ------------------------------------------------------------------------------------------------
void console_on_keyup(uint code)
{
}

// ------------------------------------------------------------------------------------------------
void console_on_char(char ch)
{
    char* line = console_get_input_line();
    uint len = strlen(line);
    if (len + 1 >= TEXT_COLS)
    {
        return;
    }

    memmove(line + s_cursor + 1, line + s_cursor, len - s_cursor);
    line[s_cursor] = ch;
    s_cursor++;
    line[len+1] = '\0';

    console_update_input();
}
