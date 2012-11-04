// ------------------------------------------------------------------------------------------------
// time/time.c
// ------------------------------------------------------------------------------------------------

#include "time/time.h"
#include "stdlib/format.h"

// ------------------------------------------------------------------------------------------------

int g_localTimeZone = -7 * 60;   // Time zone offset in minutes

// ------------------------------------------------------------------------------------------------
void SplitTime(DateTime *dt, abs_time t, int tzOffset)
{
    // Adjust t for time zone
    t += tzOffset * 60;

    // Start of each month based on day of the year
    static const int monthStart[]  = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
    static const int leapMonthStart[] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };

    // Split time into days since the epoch and seconds in that day
    int epochDays = t / (24 * 60 * 60);
    int daySecs = t % (24 * 60 * 60);

    // Compute time
    int sec = daySecs % 60;
    int min = (daySecs % 3600) / 60;
    int hour = daySecs / 3600;

    // Compute years since the epoch and days in that year
    int epochYears = (epochDays - (epochDays + 365) / 1460) / 365;
    int yearDay = epochDays - (epochYears * 365 + (epochYears + 1) / 4);

    // Adjust year based on epoch
    int year = 1970 + epochYears;

    // Search for month based on days in the year
    const int *mstart = year & 3 ? monthStart : leapMonthStart;

    int month = 1;
    while (yearDay >= mstart[month])
    {
        ++month;
    }

    // Compute day of the month and day of the week
    int day = 1 + yearDay - mstart[month - 1];
    int weekDay = (epochDays + 4) % 7;

    // Store results
    dt->sec = sec;
    dt->min = min;
    dt->hour = hour;
    dt->day = day;
    dt->month = month;
    dt->year = year;
    dt->weekDay = weekDay;
    dt->yearDay = yearDay;
    dt->tzOffset = tzOffset;
}

// ------------------------------------------------------------------------------------------------
abs_time JoinTime(const DateTime *dt)
{
    // From the Posix specification (4.14 Seconds Since the Epoch).
    // Could be simplified as the last two cases only apply starting in 2100.
    return
        dt->sec +
        dt->min * 60 +
        dt->hour * 3600 +
        dt->yearDay * 86400 +
        (dt->year - 70) * 31536000 +
        ((dt->year - 69) / 4) * 86400 -
        ((dt->year - 1) / 100) * 86400 +
        ((dt->year + 299) / 400) * 86400;
}

// ------------------------------------------------------------------------------------------------
void FormatTime(char *str, size_t size, const DateTime *dt)
{
    static const char *weekDays[] =
    {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };

    static const char *months[] =
    {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    uint w = dt->weekDay;
    uint m = dt->month - 1;

    snprintf(str, size, "%s, %02d %s %d %02d:%02d:%02d %02d%02d",
        w < 7 ? weekDays[w] : "XXX",
        dt->day,
        m < 12 ? months[m] : "XXX",
        dt->year, dt->hour, dt->min, dt->sec,
        dt->tzOffset / 60,
        dt->tzOffset % 60);
}
