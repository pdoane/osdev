// ------------------------------------------------------------------------------------------------
// rtc.c
// ------------------------------------------------------------------------------------------------

#include "rtc.h"
#include "pit.h"
#include "io.h"

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
/*
static void rtc_write(u8 addr, u8 val)
{
    out8(IO_RTC_INDEX, addr);
    out8(IO_RTC_TARGET, val);
}
*/

// ------------------------------------------------------------------------------------------------
static uint bcd_to_bin(u8 val)
{
    return (val & 0xf) + (val >> 4) * 10;
}

// ------------------------------------------------------------------------------------------------
void rtc_get_time(RTC_Time* time)
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

    // 12->24 hour conversion
    if (~regb & REGB_HOURFORM && hour & 0x80)
    {
        hour += 12;
    }

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
    time->sec = sec;
    time->min = min;
    time->hour = hour;
    time->week_day = week_day;
    time->day = day;
    time->month = month;
    time->year = year;
}
