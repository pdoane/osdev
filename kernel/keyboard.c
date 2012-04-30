// ------------------------------------------------------------------------------------------------
// keyboard.c
// ------------------------------------------------------------------------------------------------

#include "keyboard.h"
#include "console.h"
#include "idt.h"
#include "io.h"
#include "keycode.h"
#include "keymap.h"

// ------------------------------------------------------------------------------------------------
u8 keyboard_buffer[256];
u8 keyboard_read;
u8 keyboard_write;

static u8 s_flags;

// ------------------------------------------------------------------------------------------------
void keyboard_on_code(uint keycode)
{
    Keymap* keymap = &keymap_us101;

    if (s_flags & KBD_ESCAPE_SEQUENCE)
    {
        keycode |= 0x100;
        s_flags &= ~KBD_ESCAPE_SEQUENCE;
    }

    if (keycode == 0xe0 || keycode == 0xe1)
    {
        s_flags |= KBD_ESCAPE_SEQUENCE;
    }
    else
    {
        if (keycode & 0x80)
        {
            // key release
            keycode &= ~0x80;

            console_on_keyup(keycode);

            if (keycode == KEY_LSHIFT)
            {
                s_flags &= ~KBD_LSHIFT;
            }
            else if (keycode == KEY_RSHIFT)
            {
                s_flags &= ~KBD_RSHIFT;
            }
        }
        else
        {
            // key press
            console_on_keydown(keycode);

            if (keycode == KEY_LSHIFT)
            {
                s_flags |= KBD_LSHIFT;
            }
            else if (keycode == KEY_RSHIFT)
            {
                s_flags |= KBD_RSHIFT;
            }
            else if (keycode == KEY_CAPS_LOCK)
            {
                s_flags ^= KBD_CAPS_LOCK;
            }
            else if (keycode == KEY_NUM_LOCK)
            {
                s_flags ^= KBD_NUM_LOCK;
            }

            else
            {
                // character mapping
                u8 ch = 0;
                if (s_flags & KBD_NUM_LOCK)
                {
                    if (keycode < 0x80)
                    {
                        ch = keymap->numlock[keycode];
                    }
                }

                if (!ch)
                {
                    keycode &= 0x7F;
                    if (s_flags & (KBD_LSHIFT | KBD_RSHIFT))
                    {
                        ch = keymap->shift[keycode];
                    }
                    else
                    {
                        ch = keymap->base[keycode];
                    }
                }

                if (ch)
                {
                    // Apply caps lock modifier
                    if (s_flags & KBD_CAPS_LOCK)
                    {
                        if (ch >= 'a' && ch <= 'z')
                        {
                            ch += 'A' - 'a';
                        }
                        else if (ch >= 'A' && ch <= 'Z')
                        {
                            ch += 'a' - 'A';
                        }
                    }

                    console_on_char(ch);
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
void keyboard_poll()
{
    if (keyboard_read != keyboard_write)
    {
        u8 data = keyboard_buffer[keyboard_read++];

        keyboard_on_code(data);
    }
}

// ------------------------------------------------------------------------------------------------
u8 keyboard_get_flags()
{
    return s_flags;
}
