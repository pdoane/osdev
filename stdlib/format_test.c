// ------------------------------------------------------------------------------------------------
// stdlib/format_test.c
// ------------------------------------------------------------------------------------------------

#include "test/test.h"
#include "stdlib/format.h"
#include <string.h>

// ------------------------------------------------------------------------------------------------
static void test_print()
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

    // 'p'
    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%p", (void*)0x12345678), 10);
    ASSERT_EQ_STR(buf, "0x12345678");

    ASSERT_EQ_UINT(snprintf(buf, sizeof(buf), "%p", (void*)0x123456), 8);
    ASSERT_EQ_STR(buf, "0x123456");

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
}

// ------------------------------------------------------------------------------------------------
static void test_scan()
{
    int n[4];

    char* emptyFmt = "";    // workaround warning
    ASSERT_EQ_INT(sscanf("", emptyFmt), 0);
    //ASSERT_EQ_INT(sscanf("", "%d", &n[0]), -1);

    ASSERT_EQ_INT(sscanf("1", emptyFmt), 0);

    ASSERT_EQ_INT(sscanf("1", "%d", &n[0]), 1);
    ASSERT_EQ_INT(n[0], 1);
    ASSERT_EQ_INT(sscanf("2", " %d ", &n[0]), 1);
    ASSERT_EQ_INT(n[0], 2);
    ASSERT_EQ_INT(sscanf("  3 ", " %d ", &n[0]), 1);
    ASSERT_EQ_INT(n[0], 3);

    ASSERT_EQ_INT(sscanf("", "."), -1);
    ASSERT_EQ_INT(sscanf(",", "."), 0);
    ASSERT_EQ_INT(sscanf("1,", "%d.", &n[0]), 1);

    ASSERT_EQ_INT(sscanf("%1", "%%%d", &n[0]), 1);
    ASSERT_EQ_INT(n[0], 1);

    ASSERT_EQ_INT(sscanf("!1", "%%%d", &n[0]), 0);

    ASSERT_EQ_INT(sscanf("4", "%d %d", &n[0], &n[1]), 1);
    ASSERT_EQ_INT(n[0], 4);

    ASSERT_EQ_INT(sscanf("1.2.3.4", "%d.%d.%d.%d", &n[0], &n[1], &n[2], &n[3]), 4);
    ASSERT_EQ_INT(n[0], 1);
    ASSERT_EQ_INT(n[1], 2);
    ASSERT_EQ_INT(n[2], 3);
    ASSERT_EQ_INT(n[3], 4);
}

// ------------------------------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    test_print();
    test_scan();

    return EXIT_SUCCESS;
}
