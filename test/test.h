// ------------------------------------------------------------------------------------------------
// test.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include <stdbool.h>
#include <stdlib.h>

// ------------------------------------------------------------------------------------------------

void assert_true(const char* expr, bool result, const char* file, unsigned line);
void assert_eq_char(const char* expr, char result, char expected, const char* file, unsigned line);
void assert_eq_hex(const char* expr, unsigned short result, unsigned short expected, unsigned width, const char* file, unsigned line);
void assert_eq_mem(const char* expr, void* result, void* expected, size_t len, const char* file, unsigned line);
void assert_eq_ptr(const char* expr, void* result, void* expected, const char* file, unsigned line);
void assert_eq_str(const char* expr, const char* result, const char* expected, const char* file, unsigned line);
void assert_eq_int(const char* expr, long long result, long long expected, const char* file, unsigned line);
void assert_eq_uint(const char* expr, unsigned long long result, unsigned long long expected, const char* file, unsigned line);

#define ASSERT_TRUE(expr)       assert_true(#expr, expr, __FILE__, __LINE__)

#define ASSERT_EQ_CHAR(x, y)    assert_eq_char(#x, x, y, __FILE__, __LINE__)
#define ASSERT_EQ_HEX8(x, y)    assert_eq_hex(#x, x, y, 2, __FILE__, __LINE__)
#define ASSERT_EQ_HEX16(x, y)   assert_eq_hex(#x, x, y, 4, __FILE__, __LINE__)
#define ASSERT_EQ_HEX32(x, y)   assert_eq_hex(#x, x, y, 8, __FILE__, __LINE__)
#define ASSERT_EQ_HEX64(x, y)   assert_eq_hex(#x, x, y, 16, __FILE__, __LINE__)
#define ASSERT_EQ_MEM(x, y, n)  assert_eq_mem(#x, x, y, n, __FILE__, __LINE__)
#define ASSERT_EQ_PTR(x, y)     assert_eq_ptr(#x, x, y, __FILE__, __LINE__)
#define ASSERT_EQ_STR(x, y)     assert_eq_str(#x, x, y, __FILE__, __LINE__)
#define ASSERT_EQ_INT(x, y)     assert_eq_int(#x, x, y, __FILE__, __LINE__)
#define ASSERT_EQ_UINT(x, y)    assert_eq_uint(#x, x, y, __FILE__, __LINE__)

// ------------------------------------------------------------------------------------------------

typedef struct ExpectedValue
{
    struct ExpectedValue* next;
    union
    {
        char ch;
        int n;
    } u;
} ExpectedValue;

typedef struct ExpectedQueue
{
    struct ExpectedQueue* next;
    const char* function;
    const char* param;
    ExpectedValue* head;
    ExpectedValue** tail;
} ExpectedQueue;

void init_queue(ExpectedQueue* q, const char* function, const char* param);
void mock_failure();
void match_int(ExpectedQueue* q, int n);
void match_char(ExpectedQueue* q, char ch);
void expect_char(ExpectedQueue* q, char ch);
void expect_int(ExpectedQueue* q, int n);
void end_queue(ExpectedQueue* q);
void pre_mock(const char* file, unsigned line);
void post_mock();

#define MATCH_CHAR(function, param) match_char(&Mock.function##param, param)
#define MATCH_INT(function, param) match_int(&Mock.function##param, param)

#define EXPECT_CHAR(function, param) expect_char(&Mock.function##param, param)
#define EXPECT_INT(function, param) expect_int(&Mock.function##param, param)

#define DECLARE_QUEUE(function, param) ExpectedQueue function##param

#define INIT_QUEUE(function, param) init_queue(&Mock.function##param, #function, #param)

#define MOCK_BEGIN pre_mock(__FILE__, __LINE__)
#define MOCK_END post_mock()

#define RUN_MOCK(x)  MOCK_BEGIN; x; MOCK_END
#define EXPECT(x) expect_##x
