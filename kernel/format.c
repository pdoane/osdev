// ------------------------------------------------------------------------------------------------
// format.c
// ------------------------------------------------------------------------------------------------

#include "format.h"
#include "types.h"

// ------------------------------------------------------------------------------------------------
int vsnprintf(char* str, size_t size, const char* fmt, va_list args)
{
    char buf[32];

    char* p = str;
    char* end = str + size - 1;

#define OUTPUT(c) do { *p++ = (c); if (p == end) goto exit; } while(0)
#define OUTPUT_STRING(s) while ((c = *s) != 0) { OUTPUT(c); ++s; }

    for (;;)
    {
        // Read next character
        char c = *fmt++;
        if (!c)
        {
            break;
        }

        // Output non-format character
        if (c != '%')
        {
            OUTPUT(c);
            continue;
        }

        // Parse type
        char type = *fmt++;

        switch (type)
        {
        case '%':
            OUTPUT('%');
            break;

        case 'c':
            c = va_arg(args, int);
            OUTPUT(c);
            break;

        case 's':
            {
                char* s = va_arg(args, char*);
                OUTPUT_STRING(s);
            }
            break;

        case 'd':
            {
                int n = va_arg(args, int);
                if (n < 0)
                {
                    OUTPUT('-');
                    n = -n;
                }

                char* s = buf + sizeof(buf) - 1;
                *s = '\0';

                do
                {
                    c = '0' + (n % 10);
                    *--s = c;
                    n /= 10;
                } while (n > 0);

                OUTPUT_STRING(s);
            }
            break;

        case 'x':
        case 'X':
            {
                uint n = va_arg(args, uint);

                char* s = buf + sizeof(buf) - 1;
                *s = '\0';

                do
                {
                    uint digit = n & 0xf;
                    if (digit < 10)
                    {
                        c = '0' + digit;
                    }
                    else if (type == 'x')
                    {
                        c = 'a' + digit - 10;
                    }
                    else
                    {
                        c = 'A' + digit - 10;
                    }

                    *--s = c;
                    n >>= 4;
                } while (n > 0);

                OUTPUT_STRING(s);
            }
        }
    }

#undef OUTPUT
#undef OUTPUT_STRING

exit:
    *p = '\0';
    return p - str;
}

// ------------------------------------------------------------------------------------------------
int snprintf(char* str, size_t size, const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(str, size, fmt, args);
    va_end(args);

    return len;
}
