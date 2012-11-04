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
static u8 RtcRead(u8 addr)
{
    IoWrite8(IO_RTC_INDEX, addr);
    return IoRead8(IO_RTC_TARGET);
}

// ------------------------------------------------------------------------------------------------
static void RtcWrite(u8 addr, u8 val)
{
    IoWrite8(IO_RTC_INDEX, addr);
    IoWrite8(IO_RTC_TARGET, val);
}

// ------------------------------------------------------------------------------------------------
static u8 BcdToBin(u8 val)
{
    return (val & 0xf) + (val >> 4) * 10;
}

// ------------------------------------------------------------------------------------------------
static u8 BinToBcd(u8 val)
{
    return ((val / 10) << 4) + (val % 10);
}

// ------------------------------------------------------------------------------------------------
void RtcGetTime(DateTime *dt)
{
    // Wait if update is in progress
    if (RtcRead(REG_A) & REGA_UIP)
    {
        PitWait(3);    // up to 488us before update occurs + 1984us to complete
    }

    // Read Registers
    u8 sec = RtcRead(REG_SEC);
    u8 min = RtcRead(REG_MIN);
    u8 hour = RtcRead(REG_HOUR);
    u8 weekDay = RtcRead(REG_WEEK_DAY);
    u8 day = RtcRead(REG_DAY);
    u8 month = RtcRead(REG_MONTH);
    u16 year = RtcRead(REG_YEAR);

    // Get Data configuration
    u8 regb = RtcRead(REG_B);

    // BCD conversion
    if (~regb & REGB_DM)
    {
        sec = BcdToBin(sec);
        min = BcdToBin(min);
        hour = BcdToBin(hour);
        day = BcdToBin(day);
        month = BcdToBin(month);
        year = BcdToBin(year);
    }

    // Century support
    year += 2000;

    // Week day conversion: Sunday as the first day of the week (0-6)
    weekDay--;

    // Write results
    dt->sec = sec;
    dt->min = min;
    dt->hour = hour;
    dt->day = day;
    dt->month = month;
    dt->year = year;
    dt->weekDay = weekDay;
    dt->tzOffset = g_localTimeZone;
}

// ------------------------------------------------------------------------------------------------
void RtcSetTime(const DateTime *dt)
{
    u8 sec = dt->sec;
    u8 min = dt->min;
    u8 hour = dt->hour;
    u8 day = dt->day;
    u8 month = dt->month;
    u8 year = dt->year - 2000;
    u8 weekDay = dt->weekDay + 1;

    // Validate data
    if (sec >= 60 || min >= 60 || hour >= 24 || day > 31 || month > 12 || year >= 100 || weekDay > 7)
    {
        ConsolePrint("RtcSetTime: bad data\n");
        return;
    }

    // Get Data configuration
    u8 regb = RtcRead(REG_B);

    // BCD conversion
    if (~regb & REGB_DM)
    {
        sec = BinToBcd(sec);
        min = BinToBcd(min);
        hour = BinToBcd(hour);
        day = BinToBcd(day);
        month = BinToBcd(month);
        year = BinToBcd(year);
    }

    // Wait if update is in progress
    if (RtcRead(REG_A) & REGA_UIP)
    {
        PitWait(3);    // up to 488us before update occurs + 1984us to complete
    }

    // Write Registers
    RtcWrite(REG_SEC, sec);
    RtcWrite(REG_MIN, min);
    RtcWrite(REG_HOUR, hour);
    RtcWrite(REG_WEEK_DAY, weekDay);
    RtcWrite(REG_DAY, day);
    RtcWrite(REG_MONTH, month);
    RtcWrite(REG_YEAR, year);
}
