// ------------------------------------------------------------------------------------------------
// keyboard_test.c
// ------------------------------------------------------------------------------------------------

#include "test/test.h"
#include "keyboard.h"
#include "keycode.h"
#include "console_mock.h"

void keyboard_on_code(uint keycode);

// ------------------------------------------------------------------------------------------------
void idt_set_handler(u8 index, u16 type, void (*handler)())
{
}

// ------------------------------------------------------------------------------------------------
void keyboard_interrupt()
{
}

// ------------------------------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    mock_console_init();

    // key press
    EXPECT(console_on_keydown(KEY_A));
    EXPECT(console_on_char('a'));
    RUN_MOCK(keyboard_on_code(KEY_A));
    ASSERT_EQ_HEX8(keyboard_get_flags(), 0);

    EXPECT(console_on_keyup(KEY_A));
    RUN_MOCK(keyboard_on_code(0x80 | KEY_A));
    ASSERT_EQ_HEX8(keyboard_get_flags(), 0);

    // shift down
    EXPECT(console_on_keydown(KEY_LSHIFT));
    RUN_MOCK(keyboard_on_code(KEY_LSHIFT));
    ASSERT_EQ_HEX8(keyboard_get_flags(), KBD_LSHIFT);

    // key press with shift
    EXPECT(console_on_keydown(KEY_A));
    EXPECT(console_on_char('A'));
    RUN_MOCK(keyboard_on_code(KEY_A));
    ASSERT_EQ_HEX8(keyboard_get_flags(), KBD_LSHIFT);

    EXPECT(console_on_keyup(KEY_A));
    RUN_MOCK(keyboard_on_code(0x80 | KEY_A));
    ASSERT_EQ_HEX8(keyboard_get_flags(), KBD_LSHIFT);

    // shift up
    EXPECT(console_on_keyup(KEY_LSHIFT));
    RUN_MOCK(keyboard_on_code(0x80 | KEY_LSHIFT));
    ASSERT_EQ_HEX8(keyboard_get_flags(), 0);

    // caps-lock key press
    EXPECT(console_on_keydown(KEY_CAPS_LOCK));
    RUN_MOCK(keyboard_on_code(KEY_CAPS_LOCK));
    ASSERT_EQ_HEX8(keyboard_get_flags(), KBD_CAPS_LOCK);

    EXPECT(console_on_keyup(KEY_CAPS_LOCK));
    RUN_MOCK(keyboard_on_code(0x80 | KEY_CAPS_LOCK));
    ASSERT_EQ_HEX8(keyboard_get_flags(), KBD_CAPS_LOCK);

    // key press with caps-lock
    EXPECT(console_on_keydown(KEY_A));
    EXPECT(console_on_char('A'));
    RUN_MOCK(keyboard_on_code(KEY_A));
    ASSERT_EQ_HEX8(keyboard_get_flags(), KBD_CAPS_LOCK);

    // caps-lock key press
    EXPECT(console_on_keydown(KEY_CAPS_LOCK));
    RUN_MOCK(keyboard_on_code(KEY_CAPS_LOCK));
    ASSERT_EQ_HEX8(keyboard_get_flags(), 0);

    EXPECT(console_on_keyup(KEY_CAPS_LOCK));
    RUN_MOCK(keyboard_on_code(0x80 | KEY_CAPS_LOCK));
    ASSERT_EQ_HEX8(keyboard_get_flags(), 0);

    // numpad key press
    EXPECT(console_on_keydown(KEY_KP4));
    RUN_MOCK(keyboard_on_code(KEY_KP4));
    ASSERT_EQ_HEX8(keyboard_get_flags(), 0);

    EXPECT(console_on_keyup(KEY_KP4));
    RUN_MOCK(keyboard_on_code(0x80 | KEY_KP4));
    ASSERT_EQ_HEX8(keyboard_get_flags(), 0);

    // num-lock key press
    EXPECT(console_on_keydown(KEY_NUM_LOCK));
    RUN_MOCK(keyboard_on_code(KEY_NUM_LOCK));
    ASSERT_EQ_HEX8(keyboard_get_flags(), KBD_NUM_LOCK);

    EXPECT(console_on_keyup(KEY_NUM_LOCK));
    RUN_MOCK(keyboard_on_code(0x80 | KEY_NUM_LOCK));
    ASSERT_EQ_HEX8(keyboard_get_flags(), KBD_NUM_LOCK);

    // numpad key press with num-lock
    EXPECT(console_on_keydown(KEY_KP4));
    EXPECT(console_on_char('4'));
    RUN_MOCK(keyboard_on_code(KEY_KP4));
    ASSERT_EQ_HEX8(keyboard_get_flags(), KBD_NUM_LOCK);

    EXPECT(console_on_keyup(KEY_KP4));
    RUN_MOCK(keyboard_on_code(0x80 | KEY_KP4));
    ASSERT_EQ_HEX8(keyboard_get_flags(), KBD_NUM_LOCK);

    // num-lock key press
    EXPECT(console_on_keydown(KEY_NUM_LOCK));
    RUN_MOCK(keyboard_on_code(KEY_NUM_LOCK));
    ASSERT_EQ_HEX8(keyboard_get_flags(), 0);

    EXPECT(console_on_keyup(KEY_NUM_LOCK));
    RUN_MOCK(keyboard_on_code(0x80 | KEY_NUM_LOCK));
    ASSERT_EQ_HEX8(keyboard_get_flags(), 0);

    // escape sequence key press
    RUN_MOCK(keyboard_on_code(0xe0));
    ASSERT_EQ_HEX8(keyboard_get_flags(), KBD_ESCAPE_SEQUENCE);

    EXPECT(console_on_keydown(KEY_ENTER));
    RUN_MOCK(keyboard_on_code(KEY_RETURN));
    ASSERT_EQ_HEX8(keyboard_get_flags(), 0);

    RUN_MOCK(keyboard_on_code(0xe0));
    ASSERT_EQ_HEX8(keyboard_get_flags(), KBD_ESCAPE_SEQUENCE);

    EXPECT(console_on_keyup(KEY_ENTER));
    RUN_MOCK(keyboard_on_code(0x80 | KEY_RETURN));
    ASSERT_EQ_HEX8(keyboard_get_flags(), 0);

    return EXIT_SUCCESS;
}
