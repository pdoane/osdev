// ------------------------------------------------------------------------------------------------
// console/console.c
// ------------------------------------------------------------------------------------------------

#include "console/console.h"
#include "console/cmd.h"
#include "gfx/vga.h"
#include "input/keycode.h"
#include "mem/lowmem.h"
#include "stdlib/format.h"
#include "stdlib/string.h"

#define MAX_HISTORY_SIZE                16

// ------------------------------------------------------------------------------------------------
// History Line

typedef struct HistoryLine
{
    char original[TEXT_COLS];
    char edit[TEXT_COLS];
    bool valid;
} HistoryLine;

// ------------------------------------------------------------------------------------------------
// Console State

static uint s_col;
static uint s_row;

static uint s_cursor;
static char s_input_line[TEXT_COLS];
static uint s_line_index;

static HistoryLine s_history[MAX_HISTORY_SIZE];
static uint s_history_count;

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
static void console_exec()
{
    // Save current line
    char line[TEXT_COLS];
    strcpy(line, console_get_input_line());

    // Skip empty commands
    if (!line[0])
    {
        return;
    }

    // If editing an old entry, restore it to the original state
    if (s_line_index > 0)
    {
        HistoryLine* update = &s_history[s_history_count - s_line_index];
        strcpy(update->edit, update->original);
    }

    // Remove last history entry at maximum size
    if (s_history_count == MAX_HISTORY_SIZE)
    {
        memcpy(s_history, s_history + 1, sizeof(HistoryLine) * (MAX_HISTORY_SIZE - 1));
        --s_history_count;
    }

    // Add new history entry
    HistoryLine* new_line = &s_history[s_history_count];
    ++s_history_count;
    strcpy(new_line->original, line);
    strcpy(new_line->edit, line);

    // Echo input
    console_print("\n$ %s\n", line);

    // Update input state
    s_input_line[0] = '\0';
    s_line_index = 0;
    s_cursor = 0;

    // Split command line arguments
    uint argc = 0;
    const char* argv[16];

    bool in_arg = false;
    char* p = line;
    for (;;)
    {
        char ch = *p;
        if (!ch)
        {
            break;
        }

        bool is_space = ch == ' ' || ch == '\t';
        if (in_arg)
        {
            if (is_space)
            {
                *p = '\0';
                in_arg = false;
            }
        }
        else
        {
            if (!is_space)
            {
                if (argc < 16)
                {
                    argv[argc] = p;
                    ++argc;
                }

                in_arg = true;
            }
        }

        ++p;
    }

    // Execute command
    if (argc > 0)
    {
        const ConsoleCmd* cmd = console_cmd_table;
        while (cmd->name)
        {
            if (strcmp(cmd->name, argv[0]) == 0)
            {
                cmd->exec(argc, argv);
                return;
            }

            ++cmd;
        }

        console_print("Unknown command %s\n", argv[0]);
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
    return s_line_index ? s_history[s_history_count - s_line_index].edit : s_input_line;
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

    case KEY_KP_DEC: // QEMU does not seem to send the right code.
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
        console_exec();
        break;

    case KEY_KP8: // QEMU does not seem to send the right code.
    case KEY_UP:
        if (s_line_index < s_history_count)
        {
            ++s_line_index;
            line = console_get_input_line();
            s_cursor = strlen(line);
        }
        break;

    case KEY_KP2: // QEMU does not seem to send the right code.
    case KEY_DOWN:
        if (s_line_index > 0)
        {
            --s_line_index;
            line = console_get_input_line();
            s_cursor = strlen(line);
        }
        break;

    case KEY_KP4: // QEMU does not seem to send the right code.
    case KEY_LEFT:
        if (s_cursor > 0)
        {
            s_cursor --;
        }
        break;

    case KEY_KP6: // QEMU does not seem to send the right code.
    case KEY_RIGHT:
        line = console_get_input_line();
        len = strlen(line);
        if (s_cursor < len)
        {
            s_cursor ++;
        }
        break;

    case KEY_KP7: // QEMU does not seem to send the right code.
    case KEY_HOME:
        s_cursor = 0;
        break;

    case KEY_KP1: // QEMU does not seem to send the right code.
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
