// ------------------------------------------------------------------------------------------------
// format_test.c
// ------------------------------------------------------------------------------------------------

#include "test/test.h"
#include "format.h"
#include <string.h>

// ------------------------------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    char buf[16];

    // empty format
    char* emptyFmt = "";    // workaround warning
    expect(snprintf(buf, sizeof(buf), emptyFmt) == 0);
    expect(strcmp(buf, emptyFmt) == 0);

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

    // 'd'
    expect(snprintf(buf, sizeof(buf), "%d", 0) == 1);
    expect(strcmp(buf, "0") == 0);

    expect(snprintf(buf, sizeof(buf), "%d", -1) == 2);
    expect(strcmp(buf, "-1") == 0);

    expect(snprintf(buf, sizeof(buf), "%d", 1) == 1);
    expect(strcmp(buf, "1") == 0);

    expect(snprintf(buf, sizeof(buf), "%d", 3141) == 4);
    expect(strcmp(buf, "3141") == 0);

    // 'x'
    expect(snprintf(buf, sizeof(buf), "%x", 0xabcd3141) == 8);
    expect(strcmp(buf, "abcd3141") == 0);

    // 'X'
    expect(snprintf(buf, sizeof(buf), "%X", 0xabcd3141) == 8);
    expect(strcmp(buf, "ABCD3141") == 0);

    return 0;
}
