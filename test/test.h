// ------------------------------------------------------------------------------------------------
// test.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include <stdio.h>
#include <stdlib.h>

// ------------------------------------------------------------------------------------------------
static void _expect(const char* msg, const char* file, unsigned line)
{
    fprintf(stderr, "Expect failed: %s, file %s, line %d\n", msg, file, line);
    exit(EXIT_FAILURE);
}

#define expect(expr) (void)((!!(expr)) || (_expect(#expr, __FILE__, __LINE__), 0))
