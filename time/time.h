// ------------------------------------------------------------------------------------------------
// time/time.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
// Elapsed Time

typedef i32 time_t;     // seconds since Jan 1, 1970 at midnight UTC

// ------------------------------------------------------------------------------------------------
// Date/Time Components

typedef struct DateTime
{
    int sec;            // [0, 59]
    int min;            // [0, 59]
    int hour;           // [0, 23]
    int day;            // [1, 31]
    int month;          // [1, 12]
    int year;           // [1970, 2038]
    int week_day;       // [0, 6] sunday = 0
    int year_day;       // [0, 365]
    int tz_offset;      // offset in minutes
} DateTime;

#define TIME_STRING_SIZE    32

// ------------------------------------------------------------------------------------------------
// Globals

extern int tz_local;   // Time zone offset in minutes

// ------------------------------------------------------------------------------------------------
// Functions

void split_time(DateTime* dt, time_t t, int tz_offset);
time_t join_time(const DateTime* dt);
void format_time(char* str, size_t size, const DateTime* dt);
