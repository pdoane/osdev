// ------------------------------------------------------------------------------------------------
// input/input.c
// ------------------------------------------------------------------------------------------------

#include "input/input.h"
#include "input/keycode.h"
#include "input/keymap.h"
#include "console/console.h"

// ------------------------------------------------------------------------------------------------
int g_mouseX;
int g_mouseY;

// ------------------------------------------------------------------------------------------------
#define KBD_LSHIFT                      0x01
#define KBD_RSHIFT                      0x02
#define KBD_CAPS_LOCK                   0x04
#define KBD_NUM_LOCK                    0x08

static u8 s_flags;

// ------------------------------------------------------------------------------------------------
void InputOnKey(uint code, uint val)
{
    const Keymap *keymap = &g_keymapUs101;

    if (val)
    {
        // key press
        ConsoleOnKeyDown(code);

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

                ConsoleOnChar(ch);
            }
        }
    }
    else
    {
        // key release
        ConsoleOnKeyUp(code);

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

// ------------------------------------------------------------------------------------------------
void InputOnMouse(int dx, int dy)
{
    g_mouseX += dx;
    g_mouseY += dy;

    if (g_mouseX < 0)
    {
        g_mouseX = 0;
    }

    if (g_mouseX > 720)
    {
        g_mouseX = 720;
    }

    if (g_mouseY < 0)
    {
        g_mouseY = 0;
    }

    if (g_mouseY > 400)
    {
        g_mouseY = 400;
    }
}
