// ------------------------------------------------------------------------------------------------
// time/time.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
// Elapsed Time

typedef i32 abs_time;     // seconds since Jan 1, 1970 at midnight UTC

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
    int weekDay;        // [0, 6] sunday = 0
    int yearDay;        // [0, 365]
    int tzOffset;       // offset in minutes
} DateTime;

#define TIME_STRING_SIZE    32

// ------------------------------------------------------------------------------------------------
// Globals

extern int g_localTimeZone;   // Time zone offset in minutes

// ------------------------------------------------------------------------------------------------
// Functions

void SplitTime(DateTime *dt, abs_time t, int tzOffset);
abs_time JoinTime(const DateTime *dt);
void FormatTime(char *str, size_t size, const DateTime *dt);
