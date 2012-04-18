// ------------------------------------------------------------------------------------------------
// format_test.c
// ------------------------------------------------------------------------------------------------

#include "test/test.h"
#include "format.h"
#include <string.h>

// ------------------------------------------------------------------------------------------------
void format_test()
{
    char buf[32];

    // empty format
    char* emptyFmt = "";    // workaround warning
    expect(snprintf(buf, sizeof(buf), emptyFmt) == 0);
    expect(strcmp(buf, emptyFmt) == 0);

    // empty buffer
    buf[0] = 'x';
    expect(snprintf(buf, 0, emptyFmt) == 0);
    expect(buf[0] == 'x');

    // non-format character
    expect(snprintf(buf, sizeof(buf), "a") == 1);
    expect(strcmp(buf, "a") == 0);

    // % character
    expect(snprintf(buf, sizeof(buf), "%%") == 1);
    expect(strcmp(buf, "%") == 0);

    // 'c'
    expect(snprintf(buf, sizeof(buf), "%c", 'a') == 1);
    expect(strcmp(buf, "a") == 0);

    // 's'
    expect(snprintf(buf, sizeof(buf), "%s", "abc") == 3);
    expect(strcmp(buf, "abc") == 0);

    char* s = 0;
    expect(snprintf(buf, sizeof(buf), "%s", s) == 6);
    expect(strcmp(buf, "(null)") == 0);

    // 'd'
    expect(snprintf(buf, sizeof(buf), "%d", -1) == 2);
    expect(strcmp(buf, "-1") == 0);

    expect(snprintf(buf, sizeof(buf), "%d", 1) == 1);
    expect(strcmp(buf, "1") == 0);

    expect(snprintf(buf, sizeof(buf), "%d", 3141) == 4);
    expect(strcmp(buf, "3141") == 0);

    // u
    expect(snprintf(buf, sizeof(buf), "%u", -1) == 10);
    expect(strcmp(buf, "4294967295") == 0);

    expect(snprintf(buf, sizeof(buf), "%u", 1) == 1);
    expect(strcmp(buf, "1") == 0);

    expect(snprintf(buf, sizeof(buf), "%u", 3141) == 4);
    expect(strcmp(buf, "3141") == 0);

    // 'x'
    expect(snprintf(buf, sizeof(buf), "%x", 0xabcd3141) == 8);
    expect(strcmp(buf, "abcd3141") == 0);

    // 'X'
    expect(snprintf(buf, sizeof(buf), "%X", 0xabcd3141) == 8);
    expect(strcmp(buf, "ABCD3141") == 0);

    // 'llx'
    expect(snprintf(buf, sizeof(buf), "%llx", 0x123456789abcdefu) == 15);
    expect(strcmp(buf, "123456789abcdef") == 0);

    // padding
    expect(snprintf(buf, sizeof(buf), "%-10s", "left") == 10);
    expect(strcmp(buf, "left      ") == 0);

    expect(snprintf(buf, sizeof(buf), "%10s", "right") == 10);
    expect(strcmp(buf, "     right") == 0);

    expect(snprintf(buf, sizeof(buf), "%03d", 1) == 3);
    expect(strcmp(buf, "001") == 0);

    expect(snprintf(buf, sizeof(buf), "%-3d", 1) == 3);
    expect(strcmp(buf, "1  ") == 0);

    expect(snprintf(buf, sizeof(buf), "%3d", 1) == 3);
    expect(strcmp(buf, "  1") == 0);

    expect(snprintf(buf, sizeof(buf), "%016llx", 0x1234llu) == 16);
    expect(strcmp(buf, "0000000000001234") == 0);

    // multiple format characters
    expect(snprintf(buf, sizeof(buf), "%c %c", 'a', 'b') == 3);
    expect(strcmp(buf, "a b") == 0);
}
