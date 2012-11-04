// ------------------------------------------------------------------------------------------------
// console/console_test.c
// ------------------------------------------------------------------------------------------------

#include "test/test.h"
#include "console/console.h"
#include "console/cmd.h"
#include "input/keycode.h"
#include "gfx/vga.h"

// ------------------------------------------------------------------------------------------------
u16 g_vgaTextBase[80*25];

const ConsoleCmd g_consoleCmdTable[] =
{
    { 0, 0 },
};

// ------------------------------------------------------------------------------------------------
void VgaTextSetCursor(uint offset)
{
    ASSERT_EQ_UINT(offset - (TEXT_ROWS-1) * TEXT_COLS - 2, ConsoleGetCursor());
}

// ------------------------------------------------------------------------------------------------
int main(int argc, const char **argv)
{
    ConsoleInit();

    // initial state
    ASSERT_EQ_STR(ConsoleGetInputLine(), "");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 0);

    // put a character to the screen
    ConsolePutChar('a');
    ASSERT_EQ_HEX16(g_vgaTextBase[0], DEFAULT_TEXT_ATTR | 'a');

    // overflow the line
    for (uint i = 0; i < 100; ++i)
    {
        ConsolePutChar('a');
    }

    ConsolePutChar('\n');

    for (uint i = 0; i < TEXT_COLS; ++i)
    {
        ASSERT_EQ_HEX16(g_vgaTextBase[i], DEFAULT_TEXT_ATTR | 'a');
    }

    ASSERT_EQ_HEX16(g_vgaTextBase[TEXT_COLS], 0);

    // fill screen
    for (uint i = 1; i < TEXT_ROWS - 2; ++i)
    {
        ConsolePrint("%c\n", 'a' + i);
    }

    for (uint i = 0; i < TEXT_ROWS - 2; ++i)
    {
        ASSERT_EQ_HEX16(g_vgaTextBase[TEXT_COLS * i], DEFAULT_TEXT_ATTR | ('a' + i));
    }

    // scroll text
    ConsolePrint("%c\n", 'a' + TEXT_ROWS - 2);

    for (uint i = 0; i < TEXT_ROWS - 2; ++i)
    {
        ASSERT_EQ_HEX16(g_vgaTextBase[TEXT_COLS * i], DEFAULT_TEXT_ATTR | ('a' + i + 1));
    }

    // control on empty input
    ConsoleOnKeyDown(KEY_BACKSPACE);
    ASSERT_EQ_STR(ConsoleGetInputLine(), "");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 0);

    ConsoleOnKeyDown(KEY_DELETE);
    ASSERT_EQ_STR(ConsoleGetInputLine(), "");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 0);

    ConsoleOnKeyDown(KEY_LEFT);
    ASSERT_EQ_STR(ConsoleGetInputLine(), "");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 0);

    ConsoleOnKeyDown(KEY_RIGHT);
    ASSERT_EQ_STR(ConsoleGetInputLine(), "");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 0);

    ConsoleOnKeyDown(KEY_HOME);
    ASSERT_EQ_STR(ConsoleGetInputLine(), "");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 0);

    ConsoleOnKeyDown(KEY_END);
    ASSERT_EQ_STR(ConsoleGetInputLine(), "");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 0);

    // input characters
    ConsoleOnChar('a');
    ASSERT_EQ_STR(ConsoleGetInputLine(), "a");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 1);

    ConsoleOnChar('b');
    ASSERT_EQ_STR(ConsoleGetInputLine(), "ab");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 2);

    ConsoleOnChar('c');
    ASSERT_EQ_STR(ConsoleGetInputLine(), "abc");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 3);

    // navigation on input
    ConsoleOnKeyDown(KEY_LEFT);
    ASSERT_EQ_UINT(ConsoleGetCursor(), 2);

    ConsoleOnKeyDown(KEY_RIGHT);
    ASSERT_EQ_UINT(ConsoleGetCursor(), 3);

    ConsoleOnKeyDown(KEY_RIGHT);
    ASSERT_EQ_UINT(ConsoleGetCursor(), 3);

    ConsoleOnKeyDown(KEY_HOME);
    ASSERT_EQ_UINT(ConsoleGetCursor(), 0);

    ConsoleOnKeyDown(KEY_END);
    ASSERT_EQ_UINT(ConsoleGetCursor(), 3);

    // update input in middle
    ConsoleOnKeyDown(KEY_LEFT);
    ConsoleOnChar('d');
    ASSERT_EQ_STR(ConsoleGetInputLine(), "abdc");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 3);

    // deletion of input
    ConsoleOnKeyDown(KEY_BACKSPACE);
    ASSERT_EQ_STR(ConsoleGetInputLine(), "abc");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 2);

    ConsoleOnKeyDown(KEY_RIGHT);
    ConsoleOnKeyDown(KEY_DELETE);
    ASSERT_EQ_STR(ConsoleGetInputLine(), "abc");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 3);

    ConsoleOnKeyDown(KEY_BACKSPACE);
    ASSERT_EQ_STR(ConsoleGetInputLine(), "ab");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 2);

    ConsoleOnKeyDown(KEY_LEFT);
    ConsoleOnKeyDown(KEY_LEFT);
    ASSERT_EQ_UINT(ConsoleGetCursor(), 0);

    ConsoleOnKeyDown(KEY_DELETE);
    ASSERT_EQ_STR(ConsoleGetInputLine(), "b");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 0);

    ConsoleOnKeyDown(KEY_BACKSPACE);
    ASSERT_EQ_STR(ConsoleGetInputLine(), "b");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 0);

    ConsoleOnKeyDown(KEY_DELETE);
    ASSERT_EQ_STR(ConsoleGetInputLine(), "");
    ASSERT_EQ_UINT(ConsoleGetCursor(), 0);

    return EXIT_SUCCESS;
}
