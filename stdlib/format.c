// ------------------------------------------------------------------------------------------------
// format.c
// ------------------------------------------------------------------------------------------------

#include "stdlib/format.h"

// ------------------------------------------------------------------------------------------------
enum
{
    PAD_ZERO = 1,
    PAD_LEFT = 2,
};

typedef struct Formatter
{
    char* p;
    char* end;
    uint flags;
    int width;
} Formatter;

// ------------------------------------------------------------------------------------------------
static bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c== '\n' || c == '\f' || c == '\v';
}

// ------------------------------------------------------------------------------------------------
static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

// ------------------------------------------------------------------------------------------------
static void output_char(Formatter* f, char c)
{
    if (f->p < f->end)
    {
        *f->p++ = c;
    }
}

// ------------------------------------------------------------------------------------------------
static void output_string(Formatter* f, const char* s)
{
    int width = f->width;
    char padChar = f->flags & PAD_ZERO ? '0' : ' ';

    if (~f->flags & PAD_LEFT)
    {
        while (--width >= 0)
        {
            output_char(f, padChar);
        }
    }

    while (*s)
    {
        output_char(f, *s++);
    }

    while (--width >= 0)
    {
        output_char(f, padChar);
    }
}

// ------------------------------------------------------------------------------------------------
static void output_dec(Formatter* f, unsigned long long n)
{
    char buf[32];
    char* end = buf + sizeof(buf) - 1;
    char* s = end;
    *s = '\0';

    do
    {
        char c = '0' + (n % 10);
        *--s = c;
        n /= 10;
    }
    while (n > 0);

    f->width -= end - s;
    output_string(f, s);
}

// ------------------------------------------------------------------------------------------------
static void output_hex(Formatter* f, char type, unsigned long long n)
{
    char buf[32];
    char* end = buf + sizeof(buf) - 1;
    char* s = end;
    *s = '\0';

    do
    {
        uint digit = n & 0xf;
        char c;
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
    }
    while (n > 0);

    f->width -= end - s;
    output_string(f, s);
}

// ------------------------------------------------------------------------------------------------
static void output_pointer(Formatter* f, void* p)
{
    unsigned long long n = (uintptr_t)p;
    output_hex(f, 'x', n);
}

// ------------------------------------------------------------------------------------------------
int vsnprintf(char* str, size_t size, const char* fmt, va_list args)
{
    Formatter f;
    f.p = str;
    f.end = str + size - 1;

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
            output_char(&f, c);
            continue;
        }

        // Parse type specifier
        c = *fmt++;

        // Parse flags
        f.flags = 0;
        if (c == '-')
        {
            f.flags |= PAD_LEFT;
            c = *fmt++;
        }
        else if (c == '0')
        {
            f.flags |= PAD_ZERO;
            c = *fmt++;
        }

        // Parse width
        f.width = -1;
        if (is_digit(c))
        {
            int width = 0;
            do
            {
                width = width * 10 + c - '0';
                c = *fmt++;
            }
            while (is_digit(c));

            f.width = width;
        }

        // Parse length modifier
        bool is_long_long = false;

        if (c == 'l')
        {
            c = *fmt++;
            if (c == 'l')
            {
                c = *fmt++;
                is_long_long = true;
            }
        }

        // Process type specifier
        char type = c;
        switch (type)
        {
        case '%':
            output_char(&f, '%');
            break;

        case 'c':
            c = va_arg(args, int);
            output_char(&f, c);
            break;

        case 's':
            {
                char* s = va_arg(args, char*);
                if (!s)
                {
                    s = "(null)";
                }

                if (f.width > 0)
                {
                    char* p = s;
                    while (*p)
                    {
                        ++p;
                    }

                    f.width -= p - s;
                }

                output_string(&f, s);
            }
            break;

        case 'd':
            {
                long long n;
                if (is_long_long)
                {
                    n = va_arg(args, long long);
                }
                else
                {
                    n = va_arg(args, int);
                }

                if (n < 0)
                {
                    output_char(&f, '-');
                    n = -n;
                }

                output_dec(&f, n);
            }
            break;

        case 'u':
            {
                unsigned long long n;
                if (is_long_long)
                {
                    n = va_arg(args, unsigned long long);
                }
                else
                {
                    n = va_arg(args, unsigned int);
                }

                output_dec(&f, n);
            }
            break;

        case 'x':
        case 'X':
            {
                unsigned long long n;
                if (is_long_long)
                {
                    n = va_arg(args, unsigned long long);
                }
                else
                {
                    n = va_arg(args, unsigned int);
                }

                output_hex(&f, type, n);
            }
            break;

        case 'p':
            {
                void* p = va_arg(args, void*);

                output_char(&f, '0');
                output_char(&f, 'x');
                output_pointer(&f, p);
            }
            break;
        }
    }

    if (f.p < f.end + 1)
    {
        *f.p = '\0';
    }

    return f.p - str;
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

// ------------------------------------------------------------------------------------------------
int vsscanf(const char* str, const char* fmt, va_list args)
{
    int count = 0;

    for (;;)
    {
        // Read next character
        char c = *fmt++;
        if (!c)
        {
            break;
        }
        if (is_space(c))
        {
            // Whitespace
            while (is_space(*str))
            {
                ++str;
            }
        }
        else if (c != '%')
        {
match_literal:
            // Non-format character
            if (*str == '\0')
            {
                goto end_of_input;
            }

            if (*str != c)
            {
                goto match_failure;
            }

            ++str;
        }
        else
        {
            // Parse type specifider
            c = *fmt++;

            // Process type specifier
            char type = c;
            switch (type)
            {
            case '%':
                goto match_literal;

            case 'd':
                {
                    int sign = 1;

                    c = *str++;
                    if (c == '\0')
                        goto end_of_input;

                    if (c == '-')
                    {
                        sign = -1;
                        c = *str++;
                    }

                    int n = 0;
                    while (is_digit(c))
                    {
                        n = n * 10 + c - '0';
                        c = *str++;
                    }

                    n *= sign;
                    --str;

                    int* result = va_arg(args, int*);
                    *result = n;
                    ++count;
                }
                break;
            }
        }
    }

match_failure:
    return count;

end_of_input:
    return count ? count : -1;
}

// ------------------------------------------------------------------------------------------------
int sscanf(const char* str, const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    int count = vsscanf(str, fmt, args);
    va_end(args);

    return count;
}

// ------------------------------------------------------------------------------------------------
unsigned long int strtoul(const char *nptr, char **endptr, int base)
{
    const char *pCurrentChar = nptr;

    // Skip whitespace
    while (is_space(*pCurrentChar))
    {
        ++pCurrentChar;
    }

    // Optionally there maybe a sign.
    bool neg = false;
    if (*pCurrentChar == '-')
    {
        neg = true;
        ++pCurrentChar;
    }
    else if (*pCurrentChar == '+')
    {
        ++pCurrentChar;
    }

    if (base == 0)
    {
        // detect base;
        if (*pCurrentChar == '0')
        {
            ++pCurrentChar;
            if (*pCurrentChar == 'x')
            {
                base = 16;
                ++pCurrentChar;
            }
            else
            {
                base = 8;
            }
        }
        else
        {
            base = 10;
        }
    }
    else if (base == 16)
    {
        if (*pCurrentChar == '0' && *(pCurrentChar + 1) == 'x')
        {
            pCurrentChar += 2;
        }
    }
    // Don't really need to skip leading 0 for oct.


    // I've not worried about limit error handling
    unsigned long int result = 0;
    bool done = false;
    while (!done)
    {
        char currentChar = *pCurrentChar;
        int  currentValue = 0;
        if (currentChar >= '0' && currentChar <= '9')
        {
            currentValue = currentChar - '0';
        }
        else if (currentChar >= 'a' && currentChar <= 'z')
        {
            currentValue = currentChar - 'a' + 10;
        }
        else if (currentChar >= 'A' && currentChar <= 'Z')
        {
            currentValue = currentChar - 'A' + 10;
        }
        else
        {
            done = true;
        }

        if (!done)
        {
            if (currentValue >= base)
            {
                done = true;
            }
            else
            {
                ++pCurrentChar;
                result *= base;
                result += currentValue;
            }
        }
    }

    if (neg)
    {
        result = -result;
    }

    if (endptr != 0)
    {
        *endptr = (char *)pCurrentChar;
    }

    return result;
}
