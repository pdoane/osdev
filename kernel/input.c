// ------------------------------------------------------------------------------------------------
// input.c
// ------------------------------------------------------------------------------------------------

#include "input.h"
#include "console.h"
#include "keycode.h"
#include "keymap.h"

// ------------------------------------------------------------------------------------------------
#define KBD_LSHIFT                      0x01
#define KBD_RSHIFT                      0x02
#define KBD_CAPS_LOCK                   0x04
#define KBD_NUM_LOCK                    0x08

static u8 s_flags;

// ------------------------------------------------------------------------------------------------
void input_event(uint code, uint val)
{
    Keymap* keymap = &keymap_us101;

    if (val)
    {
        // key press
        console_on_keydown(code);

        if (code == KEY_LSHIFT)
        {
            s_flags |= KBD_LSHIFT;
        }
        else if (code == KEY_RSHIFT)
        {
            s_flags |= KBD_RSHIFT;
        }
        else if (code == KEY_CAPS_LOCK)
        {
            s_flags ^= KBD_CAPS_LOCK;
        }
        else if (code == KEY_NUM_LOCK)
        {
            s_flags ^= KBD_NUM_LOCK;
        }

        else
        {
            // character mapping
            u8 ch = 0;
            if (code < 0x80)
            {
                if (s_flags & KBD_NUM_LOCK)
                {
                    ch = keymap->numlock[code];
                }

                if (!ch)
                {
                    if (s_flags & (KBD_LSHIFT | KBD_RSHIFT))
                    {
                        ch = keymap->shift[code];
                    }
                    else
                    {
                        ch = keymap->base[code];
                    }
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
    else
    {
        // key release
        console_on_keyup(code);

        if (code == KEY_LSHIFT)
        {
            s_flags &= ~KBD_LSHIFT;
        }
        else if (code == KEY_RSHIFT)
        {
            s_flags &= ~KBD_RSHIFT;
        }
    }
}
