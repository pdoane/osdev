// ------------------------------------------------------------------------------------------------
// test/test.c
// ------------------------------------------------------------------------------------------------

#include "test/test.h"
#include <stdio.h>
#include <string.h>

// ------------------------------------------------------------------------------------------------
static ExpectedQueue *s_params;
static const char *s_file;
static unsigned s_line;

// ------------------------------------------------------------------------------------------------
void AssertTrue(const char *expr, bool result, const char *file, unsigned line)
{
    if (!result)
    {
        fprintf(stderr, "%s(%d): %s\n", file, line, expr);
        exit(EXIT_FAILURE);
    }
}

// ------------------------------------------------------------------------------------------------
void AssertEqChar(const char *expr, char result, char expected, const char *file, unsigned line)
{
    if (result != expected)
    {
        fprintf(stderr, "%s(%d): %s => %c != %c\n", file, line, expr, result, expected);
        exit(EXIT_FAILURE);
    }
}

// ------------------------------------------------------------------------------------------------
void AssertEqHex(const char *expr, unsigned short result, unsigned short expected, unsigned width, const char *file, unsigned line)
{
    if (result != expected)
    {
        fprintf(stderr, "%s(%d): %s => 0x%0*x != 0x%0*x\n", file, line, expr, width, result, width, expected);
        exit(EXIT_FAILURE);
    }
}

// ------------------------------------------------------------------------------------------------
void AssertEqMem(const char *expr, void *result, void *expected, size_t len, const char *file, unsigned line)
{
    if (memcmp(result, expected, len))
    {
        fprintf(stderr, "%s(%d): %s => memory compare failed\n", file, line, expr);

        unsigned char *p = (unsigned char *)result;
        unsigned char *q = (unsigned char *)expected;
        for (size_t i = 0; i < len; ++i)
        {
            fprintf(stderr, " 0x%02x '%c' %s 0x%02x '%c'\n", p[i], p[i], p[i] == q[i] ? "==" : "!=", q[i], q[i]);
        }

        exit(EXIT_FAILURE);
    }
}

// ------------------------------------------------------------------------------------------------
void AssertEqPtr(const char *expr, void *result, void *expected, const char *file, unsigned line)
{
    if (result != expected)
    {
        fprintf(stderr, "%s(%d): %s => %p != %p\n", file, line, expr,
            result, expected);
        exit(EXIT_FAILURE);
    }
}

// ------------------------------------------------------------------------------------------------
void AssertEqStr(const char *expr, const char *result, const char *expected, const char *file, unsigned line)
{
    if (strcmp(result, expected))
    {
        fprintf(stderr, "%s(%d): %s => %s != %s\n", file, line, expr, result, expected);
        exit(EXIT_FAILURE);
    }
}

// ------------------------------------------------------------------------------------------------
void AssertEqInt(const char *expr, long long result, long long expected, const char *file, unsigned line)
{
    if (result != expected)
    {
        fprintf(stderr, "%s(%d): %s => %lld != %lld\n", file, line, expr, result, expected);
        exit(EXIT_FAILURE);
    }
}

// ------------------------------------------------------------------------------------------------
void AssertEqUint(const char *expr, unsigned long long result, unsigned long long expected, const char *file, unsigned line)
{
    if (result != expected)
    {
        fprintf(stderr, "%s(%d): %s => %llu != %llu\n", file, line, expr, result, expected);
        exit(EXIT_FAILURE);
    }
}


// ------------------------------------------------------------------------------------------------
void InitQueue(ExpectedQueue *q, const char *function, const char *param)
{
    q->next = s_params;
    q->function = function;
    q->param = param;
    q->head = 0;
    q->tail = &q->head;

    s_params = q;
}

// ------------------------------------------------------------------------------------------------
void MockFailure()
{
    fprintf(stderr, "%s(%d): Mock failed\n", s_file, s_line);
    exit(EXIT_FAILURE);
}

// ------------------------------------------------------------------------------------------------
void MatchInt(ExpectedQueue *q, int n)
{
    if (q->head == 0)
    {
        fprintf(stderr, "%s: Unexpected call\n", q->function);
        MockFailure();
    }
    else
    {
        ExpectedValue *v = q->head;
        q->head = v->next;

        if (v->u.n != n)
        {
            fprintf(stderr, "%s: %s=%d, expected %d\n", q->function, q->param, n, v->u.n);
            MockFailure();
        }
    }
}

// ------------------------------------------------------------------------------------------------
void MatchChar(ExpectedQueue *q, char ch)
{
    if (q->head == 0)
    {
        fprintf(stderr, "%s: Unexpected call\n", q->function);
        MockFailure();
    }
    else
    {
        ExpectedValue *v = q->head;
        q->head = v->next;

        if (v->u.ch != ch)
        {
            fprintf(stderr, "%s: %s=%c, expected %c\n", q->function, q->param, ch, v->u.ch);
            MockFailure();
        }

        free(v);
    }
}

// ------------------------------------------------------------------------------------------------
void ExpectChar(ExpectedQueue *q, char ch)
{
    ExpectedValue *v = (ExpectedValue *)malloc(sizeof(ExpectedValue));
    v->next = 0;
    v->u.ch = ch;
    *q->tail = v;
    q->tail = &v;
}

// ------------------------------------------------------------------------------------------------
void ExpectInt(ExpectedQueue *q, int n)
{
    ExpectedValue *v = (ExpectedValue *)malloc(sizeof(ExpectedValue));
    v->next = 0;
    v->u.n = n;
    *q->tail = v;
    q->tail = &v;
}

// ------------------------------------------------------------------------------------------------
void EndQueue(ExpectedQueue *q)
{
    if (q->head)
    {
        fprintf(stderr, "%s: Expected more calls\n", q->function);
        MockFailure();
    }

    q->tail = &q->head;
}

// ------------------------------------------------------------------------------------------------
void PreMock(const char *file, unsigned line)
{
    s_file = file;
    s_line = line;
}

// ------------------------------------------------------------------------------------------------
void PostMock()
{
    for (ExpectedQueue *q = s_params; q; q = q->next)
    {
        EndQueue(q);
    }
}
