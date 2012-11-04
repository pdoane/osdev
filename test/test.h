// ------------------------------------------------------------------------------------------------
// test/test.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include <stdbool.h>
#include <stdlib.h>

// ------------------------------------------------------------------------------------------------

void AssertTrue(const char *expr, bool result, const char *file, unsigned line);
void AssertEqChar(const char *expr, char result, char expected, const char *file, unsigned line);
void AssertEqHex(const char *expr, unsigned short result, unsigned short expected, unsigned width, const char *file, unsigned line);
void AssertEqMem(const char *expr, void *result, void *expected, size_t len, const char *file, unsigned line);
void AssertEqPtr(const char *expr, void *result, void *expected, const char *file, unsigned line);
void AssertEqStr(const char *expr, const char *result, const char *expected, const char *file, unsigned line);
void AssertEqInt(const char *expr, long long result, long long expected, const char *file, unsigned line);
void AssertEqUint(const char *expr, unsigned long long result, unsigned long long expected, const char *file, unsigned line);

#define ASSERT_TRUE(expr)       AssertTrue(#expr, expr, __FILE__, __LINE__)

#define ASSERT_EQ_CHAR(x, y)    AssertEqChar(#x, x, y, __FILE__, __LINE__)
#define ASSERT_EQ_HEX8(x, y)    AssertEqHex(#x, x, y, 2, __FILE__, __LINE__)
#define ASSERT_EQ_HEX16(x, y)   AssertEqHex(#x, x, y, 4, __FILE__, __LINE__)
#define ASSERT_EQ_HEX32(x, y)   AssertEqHex(#x, x, y, 8, __FILE__, __LINE__)
#define ASSERT_EQ_HEX64(x, y)   AssertEqHex(#x, x, y, 16, __FILE__, __LINE__)
#define ASSERT_EQ_MEM(x, y, n)  AssertEqMem(#x, x, y, n, __FILE__, __LINE__)
#define ASSERT_EQ_PTR(x, y)     AssertEqPtr(#x, x, y, __FILE__, __LINE__)
#define ASSERT_EQ_STR(x, y)     AssertEqStr(#x, x, y, __FILE__, __LINE__)
#define ASSERT_EQ_INT(x, y)     AssertEqInt(#x, x, y, __FILE__, __LINE__)
#define ASSERT_EQ_UINT(x, y)    AssertEqUint(#x, x, y, __FILE__, __LINE__)

// ------------------------------------------------------------------------------------------------

typedef struct ExpectedValue
{
    struct ExpectedValue *next;
    union
    {
        char ch;
        int n;
    } u;
} ExpectedValue;

typedef struct ExpectedQueue
{
    struct ExpectedQueue *next;
    const char *function;
    const char *param;
    ExpectedValue *head;
    ExpectedValue **tail;
} ExpectedQueue;

void InitQueue(ExpectedQueue *q, const char *function, const char *param);
void MockFailure();
void MatchInt(ExpectedQueue *q, int n);
void MatchChar(ExpectedQueue *q, char ch);
void ExpectChar(ExpectedQueue *q, char ch);
void ExpectInt(ExpectedQueue *q, int n);
void EndQueue(ExpectedQueue *q);
void PreMock(const char *file, unsigned line);
void PostMock();

#define MATCH_CHAR(function, param) MatchChar(&Mock.function##param, param)
#define MATCH_INT(function, param) MatchInt(&Mock.function##param, param)

#define EXPECT_CHAR(function, param) ExpectChar(&Mock.function##param, param)
#define EXPECT_INT(function, param) ExpectInt(&Mock.function##param, param)

#define DECLARE_QUEUE(function, param) ExpectedQueue function##param

#define INIT_QUEUE(function, param) InitQueue(&Mock.function##param, #function, #param)

#define MOCK_BEGIN PreMock(__FILE__, __LINE__)
#define MOCK_END PostMock()

#define RUN_MOCK(x)  MOCK_BEGIN; x; MOCK_END
#define EXPECT(x) expect_##x
