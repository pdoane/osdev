// ------------------------------------------------------------------------------------------------
// time/time.c
// ------------------------------------------------------------------------------------------------

#include "time/time.h"
#include "stdlib/format.h"

// ------------------------------------------------------------------------------------------------

int tz_local = -7 * 60;   // Time zone offset in minutes

// ------------------------------------------------------------------------------------------------
void split_time(DateTime* dt, abs_time t, int tz_offset)
{
    // Adjust t for time zone
    t += tz_offset * 60;

    // Start of each month based on day of the year
    static int reg_mstart[]  = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
    static int leap_mstart[] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };

    // Split time into days since the epoch and seconds in that day
    int epoch_days = t / (24 * 60 * 60);
    int day_secs = t % (24 * 60 * 60);

    // Compute time
    int sec = day_secs % 60;
    int min = (day_secs % 3600) / 60;
    int hour = day_secs / 3600;

    // Compute years since the epoch and days in that year
    int epoch_years = (epoch_days - (epoch_days + 365) / 1460) / 365;
    int year_day = epoch_days - (epoch_years * 365 + (epoch_years + 1) / 4);

    // Adjust year based on epoch
    int year = 1970 + epoch_years;

    // Search for month based on days in the year
    const int* mstart = year & 3 ? reg_mstart : leap_mstart;

    int month = 1;
    while (year_day >= mstart[month])
    {
        ++month;
    }

    // Compute day of the month and day of the week
    int day = 1 + year_day - mstart[month - 1];
    int week_day = (epoch_days + 4) % 7;

    // Store results
    dt->sec = sec;
    dt->min = min;
    dt->hour = hour;
    dt->day = day;
    dt->month = month;
    dt->year = year;
    dt->week_day = week_day;
    dt->year_day = year_day;
    dt->tz_offset = tz_offset;
}

// ------------------------------------------------------------------------------------------------
abs_time join_time(const DateTime* dt)
{
    // From the Posix specification (4.14 Seconds Since the Epoch).
    // Could be simplified as the last two cases only apply starting in 2100.
    return
        dt->sec +
        dt->min * 60 +
        dt->hour * 3600 +
        dt->year_day * 86400 +
        (dt->year - 70) * 31536000 +
        ((dt->year - 69) / 4) * 86400 -
        ((dt->year - 1) / 100) * 86400 +
        ((dt->year + 299) / 400) * 86400;
}

// ------------------------------------------------------------------------------------------------
void format_time(char* str, size_t size, const DateTime* dt)
{
    static const char* week_days[] =
    {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };

    static const char* months[] =
    {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    uint w = dt->week_day;
    uint m = dt->month - 1;

    snprintf(str, size, "%s, %02d %s %d %02d:%02d:%02d %02d%02d",
        w < 7 ? week_days[w] : "XXX",
        dt->day,
        m < 12 ? months[m] : "XXX",
        dt->year, dt->hour, dt->min, dt->sec,
        dt->tz_offset / 60,
        dt->tz_offset % 60);
}
