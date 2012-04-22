// ------------------------------------------------------------------------------------------------
// console_mock.c
// ------------------------------------------------------------------------------------------------

#include "test/test.h"
#include "console_mock.h"

// ------------------------------------------------------------------------------------------------
static struct MockData
{
    DECLARE_QUEUE(console_on_keydown, code);
    DECLARE_QUEUE(console_on_keyup, code);
    DECLARE_QUEUE(console_on_char, ch);
} Mock;

// ------------------------------------------------------------------------------------------------
void console_on_keydown(uint code)
{
    MATCH_INT(console_on_keydown, code);
}

// ------------------------------------------------------------------------------------------------
void expect_console_on_keydown(uint code)
{
    EXPECT_INT(console_on_keydown, code);
}

// ------------------------------------------------------------------------------------------------
void console_on_keyup(uint code)
{
    MATCH_INT(console_on_keyup, code);
}

// ------------------------------------------------------------------------------------------------
void expect_console_on_keyup(uint code)
{
    EXPECT_INT(console_on_keyup, code);
}

// ------------------------------------------------------------------------------------------------
void console_on_char(char ch)
{
    MATCH_CHAR(console_on_char, ch);
}

// ------------------------------------------------------------------------------------------------
void expect_console_on_char(char ch)
{
    EXPECT_CHAR(console_on_char, ch);
}

// ------------------------------------------------------------------------------------------------
void mock_console_init()
{
    INIT_QUEUE(console_on_keydown, code);
    INIT_QUEUE(console_on_keyup, code);
    INIT_QUEUE(console_on_char, ch);
}
