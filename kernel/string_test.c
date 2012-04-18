// ------------------------------------------------------------------------------------------------
// string_test.c
// ------------------------------------------------------------------------------------------------

#include "test/test.h"
#include "string.h"

// ------------------------------------------------------------------------------------------------
void string_test()
{
    char buf[128];
    for (int i = 0; i < 128; ++i)
    {
        buf[i] = i;
    }

    // memset
    expect(memset(buf, 1, 0) == buf);
    expect(buf[0] == 0);

    expect(memset(buf, 1, 3) == buf);
    expect(buf[0] == 1 && buf[1] == 1 && buf[2] == 1);

    // memcpy
    expect(memcpy(buf, buf + 3, 3) == buf);
    expect(buf[0] == 3 && buf[1] == 4 && buf[2] == 5);

    // memmove
    expect(memmove(buf + 1, buf, 3) == buf + 1);
    expect(buf[0] == 3 && buf[1] == 3 && buf[2] == 4 && buf[3] == 5);

    // strlen
    buf[0] = '\0';
    expect(strlen(buf) == 0);

    buf[0] = 'a'; buf[1] = '\0';
    expect(strlen(buf) == 1);

    buf[1] = 'b'; buf[2] = '\0';
    expect(strlen(buf) == 2);

    // strcpy
    expect(strcpy(buf, "") == buf);
    expect(strcmp(buf, "") == 0);

    expect(strcpy(buf, "foo") == buf);
    expect(strcmp(buf, "foo") == 0);

    // strncpy
    buf[0] = 1; buf[1] = 1; buf[2] = 1;

    expect(strncpy(buf, "a", 0) == buf);
    expect(buf[0] == 1);

    expect(strncpy(buf, "a", 1) == buf);
    expect(buf[0] == 'a' && buf[1] == 1 && buf[2] == 1);

    expect(strncpy(buf, "a", 2) == buf);
    expect(buf[0] == 'a' && buf[1] == 0 && buf[2] == 1);

    expect(strncpy(buf, "a", 3) == buf);
    expect(buf[0] == 'a' && buf[1] == 0 && buf[2] == 0);

    // strcmp
    expect(strcmp("", "") == 0);
    expect(strcmp("", "a") < 0);
    expect(strcmp("a", "") > 0);
    expect(strcmp("a", "b") < 0);
    expect(strcmp("b", "b") == 0);
    expect(strcmp("b", "a") > 0);
    expect(strcmp("a", "ab") < 0);
    expect(strcmp("ab", "a") > 0);
    expect(strcmp("ab", "b") < 0);
    expect(strcmp("ba", "a") > 0);

    // strcpy_safe
#ifndef NATIVE
    buf[0] = 1; buf[1] = 1; buf[2] = 1;

    expect(strcpy_safe(buf, "a", 0) == buf);
    expect(buf[0] == 1);

    expect(strcpy_safe(buf, "a", 1) == buf);
    expect(buf[0] == '\0' && buf[1] == 1 && buf[2] == 1);

    expect(strcpy_safe(buf, "a", 2) == buf);
    expect(buf[0] == 'a' && buf[1] == 0 && buf[2] == 1);

    expect(strcpy_safe(buf, "a", 3) == buf);
    expect(buf[0] == 'a' && buf[1] == 0 && buf[2] == 1);
#endif
}
