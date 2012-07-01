// ------------------------------------------------------------------------------------------------
// time/rtc.c
// ------------------------------------------------------------------------------------------------

#include "time/rtc.h"
#include "time/pit.h"
#include "console/console.h"
#include "cpu/io.h"

// ------------------------------------------------------------------------------------------------
// I/O Ports

#define IO_RTC_INDEX                    0x70
#define IO_RTC_TARGET                   0x71

// ------------------------------------------------------------------------------------------------
// Indexed Registers

#define REG_SEC                         0x00
#define REG_SEC_ALARM                   0x01
#define REG_MIN                         0x02
#define REG_MIN_ALARM                   0x03
#define REG_HOUR                        0x04
#define REG_HOUR_ALARM                  0x05
#define REG_WEEK_DAY                    0x06
#define REG_DAY                         0x07
#define REG_MONTH                       0x08
#define REG_YEAR                        0x09
#define REG_A                           0x0a
#define REG_B                           0x0b
#define REG_C                           0x0c
#define REG_D                           0x0d

// ------------------------------------------------------------------------------------------------
// General Configuration Registers

#define REGA_UIP                        (1 << 7)    // Update In Progress

#define REGB_HOURFORM                   (1 << 1)    // Hour Format (0 = 12hr, 1 = 24hr)
#define REGB_DM                         (1 << 2)    // Data Mode (0 = BCD, 1 = Binary)

// ------------------------------------------------------------------------------------------------
static u8 rtc_read(u8 addr)
{
    out8(IO_RTC_INDEX, addr);
    return in8(IO_RTC_TARGET);
}

// ------------------------------------------------------------------------------------------------
static void rtc_write(u8 addr, u8 val)
{
    out8(IO_RTC_INDEX, addr);
    out8(IO_RTC_TARGET, val);
}

// ------------------------------------------------------------------------------------------------
static u8 bcd_to_bin(u8 val)
{
    return (val & 0xf) + (val >> 4) * 10;
}

// ------------------------------------------------------------------------------------------------
static u8 bin_to_bcd(u8 val)
{
    return ((val / 10) << 4) + (val % 10);
}

// ------------------------------------------------------------------------------------------------
void rtc_get_time(DateTime* dt)
{
    // Wait if update is in progress
    if (rtc_read(REG_A) & REGA_UIP)
    {
        pit_wait(3);    // up to 488us before update occurs + 1984us to complete
    }

    // Read Registers
    u8 sec = rtc_read(REG_SEC);
    u8 min = rtc_read(REG_MIN);
    u8 hour = rtc_read(REG_HOUR);
    u8 week_day = rtc_read(REG_WEEK_DAY);
    u8 day = rtc_read(REG_DAY);
    u8 month = rtc_read(REG_MONTH);
    u16 year = rtc_read(REG_YEAR);

    // Get Data configuration
    u8 regb = rtc_read(REG_B);

    // BCD conversion
    if (~regb & REGB_DM)
    {
        sec = bcd_to_bin(sec);
        min = bcd_to_bin(min);
        hour = bcd_to_bin(hour);
        day = bcd_to_bin(day);
        month = bcd_to_bin(month);
        year = bcd_to_bin(year);
    }

    // Century support
    year += 2000;

    // Week day conversion: Sunday as the first day of the week (0-6)
    week_day--;

    // Write results
    dt->sec = sec;
    dt->min = min;
    dt->hour = hour;
    dt->day = day;
    dt->month = month;
    dt->year = year;
    dt->week_day = week_day;
    dt->tz_offset = tz_local;
}

// ------------------------------------------------------------------------------------------------
void rtc_set_time(const DateTime* dt)
{
    u8 sec = dt->sec;
    u8 min = dt->min;
    u8 hour = dt->hour;
    u8 day = dt->day;
    u8 month = dt->month;
    u8 year = dt->year - 2000;
    u8 week_day = dt->week_day + 1;

    // Validate data
    if (sec >= 60 || min >= 60 || hour >= 24 || day > 31 || month > 12 || year >= 100 || week_day > 7)
    {
        console_print("rtc_set_time: bad data\n");
        return;
    }

    // Get Data configuration
    u8 regb = rtc_read(REG_B);

    // BCD conversion
    if (~regb & REGB_DM)
    {
        sec = bin_to_bcd(sec);
        min = bin_to_bcd(min);
        hour = bin_to_bcd(hour);
        day = bin_to_bcd(day);
        month = bin_to_bcd(month);
        year = bin_to_bcd(year);
    }

    // Wait if update is in progress
    if (rtc_read(REG_A) & REGA_UIP)
    {
        pit_wait(3);    // up to 488us before update occurs + 1984us to complete
    }

    // Write Registers
    rtc_write(REG_SEC, sec);
    rtc_write(REG_MIN, min);
    rtc_write(REG_HOUR, hour);
    rtc_write(REG_WEEK_DAY, week_day);
    rtc_write(REG_DAY, day);
    rtc_write(REG_MONTH, month);
    rtc_write(REG_YEAR, year);
}
