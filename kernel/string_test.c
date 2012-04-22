// ------------------------------------------------------------------------------------------------
// string_test.c
// ------------------------------------------------------------------------------------------------

#include "test/test.h"
#include "string.h"

// ------------------------------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    char buf[128];
    for (int i = 0; i < 128; ++i)
    {
        buf[i] = i;
    }

    // memset
    ASSERT_EQ_PTR(memset(buf, 1, 0), buf);
    ASSERT_EQ_UINT(buf[0], 0);

    ASSERT_EQ_PTR(memset(buf, 1, 3), buf);
    ASSERT_TRUE(buf[0] == 1 && buf[1] == 1 && buf[2] == 1);

    // memcpy
    ASSERT_EQ_PTR(memcpy(buf, buf + 3, 3), buf);
    ASSERT_TRUE(buf[0] == 3 && buf[1] == 4 && buf[2] == 5);

    // memmove
    ASSERT_EQ_PTR(memmove(buf + 1, buf, 3), buf + 1);
    ASSERT_TRUE(buf[0] == 3 && buf[1] == 3 && buf[2] == 4 && buf[3] == 5);

    // strlen
    buf[0] = '\0';
    ASSERT_EQ_UINT(strlen(buf), 0);

    buf[0] = 'a'; buf[1] = '\0';
    ASSERT_EQ_UINT(strlen(buf), 1);

    buf[1] = 'b'; buf[2] = '\0';
    ASSERT_EQ_UINT(strlen(buf), 2);

    // strcpy
    ASSERT_EQ_PTR(strcpy(buf, ""), buf);
    ASSERT_EQ_STR(buf, "");

    ASSERT_EQ_PTR(strcpy(buf, "foo"), buf);
    ASSERT_EQ_STR(buf, "foo");

    // strncpy
    buf[0] = 1; buf[1] = 1; buf[2] = 1;

    ASSERT_EQ_PTR(strncpy(buf, "a", 0), buf);
    ASSERT_TRUE(buf[0] == 1);

    ASSERT_EQ_PTR(strncpy(buf, "a", 1), buf);
    ASSERT_TRUE(buf[0] == 'a' && buf[1] == 1 && buf[2] == 1);

    ASSERT_EQ_PTR(strncpy(buf, "a", 2), buf);
    ASSERT_TRUE(buf[0] == 'a' && buf[1] == 0 && buf[2] == 1);

    ASSERT_EQ_PTR(strncpy(buf, "a", 3), buf);
    ASSERT_TRUE(buf[0] == 'a' && buf[1] == 0 && buf[2] == 0);

    // strcmp
    ASSERT_TRUE(strcmp("", "") == 0);
    ASSERT_TRUE(strcmp("", "a") < 0);
    ASSERT_TRUE(strcmp("a", "") > 0);
    ASSERT_TRUE(strcmp("a", "b") < 0);
    ASSERT_TRUE(strcmp("b", "b") == 0);
    ASSERT_TRUE(strcmp("b", "a") > 0);
    ASSERT_TRUE(strcmp("a", "ab") < 0);
    ASSERT_TRUE(strcmp("ab", "a") > 0);
    ASSERT_TRUE(strcmp("ab", "b") < 0);
    ASSERT_TRUE(strcmp("ba", "a") > 0);

    // strcpy_safe
#ifndef NATIVE
    buf[0] = 1; buf[1] = 1; buf[2] = 1;

    ASSERT_EQ_PTR(strcpy_safe(buf, "a", 0), buf);
    ASSERT_TRUE(buf[0] == 1);

    ASSERT_EQ_PTR(strcpy_safe(buf, "a", 1), buf);
    ASSERT_TRUE(buf[0] == '\0' && buf[1] == 1 && buf[2] == 1);

    ASSERT_EQ_PTR(strcpy_safe(buf, "a", 2), buf);
    ASSERT_TRUE(buf[0] == 'a' && buf[1] == 0 && buf[2] == 1);

    ASSERT_EQ_PTR(strcpy_safe(buf, "a", 3), buf);
    ASSERT_TRUE(buf[0] == 'a' && buf[1] == 0 && buf[2] == 1);
#endif

    return EXIT_SUCCESS;
}
