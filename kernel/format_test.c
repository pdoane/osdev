// ------------------------------------------------------------------------------------------------
// format_test.c
// ------------------------------------------------------------------------------------------------

#include "test/test.h"
#include "format.h"
#include <string.h>

// ------------------------------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    char buf[32];

    // empty format
    char* emptyFmt = "";    // workaround warning
    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), emptyFmt), 0);
    ASSERT_EQ_STR(buf, emptyFmt);

    // empty buffer
    buf[0] = 'x';
    ASSERT_EQ_UINT(snprintf(buf, 0, emptyFmt), 0);
    ASSERT_EQ_CHAR(buf[0], 'x');

    // non-format character
    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "a"), 1);
    ASSERT_EQ_STR(buf, "a");

    // % character
    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%%"), 1);
    ASSERT_EQ_STR(buf, "%");

    // 'c'
    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%c", 'a'), 1);
    ASSERT_EQ_STR(buf, "a");

    // 's'
    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%s", "abc"), 3);
    ASSERT_EQ_STR(buf, "abc");

    char* s = 0;
    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%s", s), 6);
    ASSERT_EQ_STR(buf, "(null)");

    // 'd'
    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%d", -1), 2);
    ASSERT_EQ_STR(buf, "-1");

    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%d", 1), 1);
    ASSERT_EQ_STR(buf, "1");

    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%d", 3141), 4);
    ASSERT_EQ_STR(buf, "3141");

    // u
    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%u", -1), 10);
    ASSERT_EQ_STR(buf, "4294967295");

    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%u", 1), 1);
    ASSERT_EQ_STR(buf, "1");

    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%u", 3141), 4);
    ASSERT_EQ_STR(buf, "3141");

    // 'x'
    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%x", 0xabcd3141), 8);
    ASSERT_EQ_STR(buf, "abcd3141");

    // 'X'
    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%X", 0xabcd3141), 8);
    ASSERT_EQ_STR(buf, "ABCD3141");

    // 'llx'
    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%llx", 0x123456789abcdefu), 15);
    ASSERT_EQ_STR(buf, "123456789abcdef");

    // padding
    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%-10s", "left"), 10);
    ASSERT_EQ_STR(buf, "left      ");

    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%10s", "right"), 10);
    ASSERT_EQ_STR(buf, "     right");

    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%03d", 1), 3);
    ASSERT_EQ_STR(buf, "001");

    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%-3d", 1), 3);
    ASSERT_EQ_STR(buf, "1  ");

    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%3d", 1), 3);
    ASSERT_EQ_STR(buf, "  1");

    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%016llx", 0x1234llu), 16);
    ASSERT_EQ_STR(buf, "0000000000001234");

    // multiple format characters
    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%c %c", 'a', 'b'), 3);
    ASSERT_EQ_STR(buf, "a b");

    return EXIT_SUCCESS;
}
