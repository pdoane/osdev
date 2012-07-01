// ------------------------------------------------------------------------------------------------
// time/pit.c
// ------------------------------------------------------------------------------------------------

#include "time/pit.h"
#include "cpu/io.h"

volatile u32 pit_ticks;

// ------------------------------------------------------------------------------------------------
// I/O Ports
#define PIT_COUNTER0                    0x40
#define PIT_CMD                         0x43

// ------------------------------------------------------------------------------------------------
// Command Register

// BCD
#define CMD_BINARY                      0x00    // Use Binary counter values
#define CMD_BCD                         0x01    // Use Binary Coded Decimal counter values

// Mode
#define CMD_MODE0                       0x00    // Interrupt on Terminal Count
#define CMD_MODE1                       0x02    // Hardware Retriggerable One-Shot
#define CMD_MODE2                       0x04    // Rate Generator
#define CMD_MODE3                       0x06    // Square Wave
#define CMD_MODE4                       0x08    // Software Trigerred Strobe
#define CMD_MODE5                       0x0a    // Hardware Trigerred Strobe

// Read/Write
#define CMD_LATCH                       0x00
#define CMD_RW_LOW                      0x10    // Least Significant Byte
#define CMD_RW_HI                       0x20    // Most Significant Byte
#define CMD_RW_BOTH                     0x30    // Least followed by Most Significant Byte

// Counter Select
#define CMD_COUNTER0                    0x00
#define CMD_COUNTER1                    0x40
#define CMD_COUNTER2                    0x80
#define CMD_READBACK                    0xc0

// ------------------------------------------------------------------------------------------------

#define PIT_FREQUENCY                   1193182

// ------------------------------------------------------------------------------------------------
void pit_init()
{
    uint hz = 1000;
    uint divisor = PIT_FREQUENCY / hz;
    out8(PIT_CMD, CMD_BINARY | CMD_MODE3 | CMD_RW_BOTH | CMD_COUNTER0);
    out8(PIT_COUNTER0, divisor);
    out8(PIT_COUNTER0, divisor >> 8);
}

// ------------------------------------------------------------------------------------------------
void pit_wait(uint ms)
{
    u32 now = pit_ticks;
    ++ms;

    while (pit_ticks - now < ms)
    {
        ;
    }
}
