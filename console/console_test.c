// ------------------------------------------------------------------------------------------------
// console/console_test.c
// ------------------------------------------------------------------------------------------------

#include "test/test.h"
#include "console/console.h"
#include "console/cmd.h"
#include "input/keycode.h"
#include "gfx/vga.h"

u16 vga_text_base[80*25];

ConsoleCmd console_cmd_table[] =
{
    { 0, 0 },
};

// ------------------------------------------------------------------------------------------------
void vga_text_setcursor(uint offset)
{
    ASSERT_EQ_UINT(offset - (TEXT_ROWS-1) * TEXT_COLS - 2, console_get_cursor());
}

// ------------------------------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    console_init();

    // initial state
    ASSERT_EQ_STR(console_get_input_line(), "");
    ASSERT_EQ_UINT(console_get_cursor(), 0);

    // put a character to the screen
    console_putchar('a');
    ASSERT_EQ_HEX16(vga_text_base[0], DEFAULT_TEXT_ATTR | 'a');

    // overflow the line
    for (uint i = 0; i < 100; ++i)
    {
        console_putchar('a');
    }

    console_putchar('\n');

    for (uint i = 0; i < TEXT_COLS; ++i)
    {
        ASSERT_EQ_HEX16(vga_text_base[i], DEFAULT_TEXT_ATTR | 'a');
    }

    ASSERT_EQ_HEX16(vga_text_base[TEXT_COLS], 0);

    // fill screen
    for (uint i = 1; i < TEXT_ROWS - 2; ++i)
    {
        console_print("%c\n", 'a' + i);
    }

    for (uint i = 0; i < TEXT_ROWS - 2; ++i)
    {
        ASSERT_EQ_HEX16(vga_text_base[TEXT_COLS * i], DEFAULT_TEXT_ATTR | ('a' + i));
    }

    // scroll text
    console_print("%c\n", 'a' + TEXT_ROWS - 2);

    for (uint i = 0; i < TEXT_ROWS - 2; ++i)
    {
        ASSERT_EQ_HEX16(vga_text_base[TEXT_COLS * i], DEFAULT_TEXT_ATTR | ('a' + i + 1));
    }

    // control on empty input
    console_on_keydown(KEY_BACKSPACE);
    ASSERT_EQ_STR(console_get_input_line(), "");
    ASSERT_EQ_UINT(console_get_cursor(), 0);

    console_on_keydown(KEY_DELETE);
    ASSERT_EQ_STR(console_get_input_line(), "");
    ASSERT_EQ_UINT(console_get_cursor(), 0);

    console_on_keydown(KEY_LEFT);
    ASSERT_EQ_STR(console_get_input_line(), "");
    ASSERT_EQ_UINT(console_get_cursor(), 0);

    console_on_keydown(KEY_RIGHT);
    ASSERT_EQ_STR(console_get_input_line(), "");
    ASSERT_EQ_UINT(console_get_cursor(), 0);

    console_on_keydown(KEY_HOME);
    ASSERT_EQ_STR(console_get_input_line(), "");
    ASSERT_EQ_UINT(console_get_cursor(), 0);

    console_on_keydown(KEY_END);
    ASSERT_EQ_STR(console_get_input_line(), "");
    ASSERT_EQ_UINT(console_get_cursor(), 0);

    // input characters
    console_on_char('a');
    ASSERT_EQ_STR(console_get_input_line(), "a");
    ASSERT_EQ_UINT(console_get_cursor(), 1);

    console_on_char('b');
    ASSERT_EQ_STR(console_get_input_line(), "ab");
    ASSERT_EQ_UINT(console_get_cursor(), 2);

    console_on_char('c');
    ASSERT_EQ_STR(console_get_input_line(), "abc");
    ASSERT_EQ_UINT(console_get_cursor(), 3);

    // navigation on input
    console_on_keydown(KEY_LEFT);
    ASSERT_EQ_UINT(console_get_cursor(), 2);

    console_on_keydown(KEY_RIGHT);
    ASSERT_EQ_UINT(console_get_cursor(), 3);

    console_on_keydown(KEY_RIGHT);
    ASSERT_EQ_UINT(console_get_cursor(), 3);

    console_on_keydown(KEY_HOME);
    ASSERT_EQ_UINT(console_get_cursor(), 0);

    console_on_keydown(KEY_END);
    ASSERT_EQ_UINT(console_get_cursor(), 3);

    // update input in middle
    console_on_keydown(KEY_LEFT);
    console_on_char('d');
    ASSERT_EQ_STR(console_get_input_line(), "abdc");
    ASSERT_EQ_UINT(console_get_cursor(), 3);

    // deletion of input
    console_on_keydown(KEY_BACKSPACE);
    ASSERT_EQ_STR(console_get_input_line(), "abc");
    ASSERT_EQ_UINT(console_get_cursor(), 2);

    console_on_keydown(KEY_RIGHT);
    console_on_keydown(KEY_DELETE);
    ASSERT_EQ_STR(console_get_input_line(), "abc");
    ASSERT_EQ_UINT(console_get_cursor(), 3);

    console_on_keydown(KEY_BACKSPACE);
    ASSERT_EQ_STR(console_get_input_line(), "ab");
    ASSERT_EQ_UINT(console_get_cursor(), 2);

    console_on_keydown(KEY_LEFT);
    console_on_keydown(KEY_LEFT);
    ASSERT_EQ_UINT(console_get_cursor(), 0);

    console_on_keydown(KEY_DELETE);
    ASSERT_EQ_STR(console_get_input_line(), "b");
    ASSERT_EQ_UINT(console_get_cursor(), 0);

    console_on_keydown(KEY_BACKSPACE);
    ASSERT_EQ_STR(console_get_input_line(), "b");
    ASSERT_EQ_UINT(console_get_cursor(), 0);

    console_on_keydown(KEY_DELETE);
    ASSERT_EQ_STR(console_get_input_line(), "");
    ASSERT_EQ_UINT(console_get_cursor(), 0);

    return EXIT_SUCCESS;
}
