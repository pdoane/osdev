// ------------------------------------------------------------------------------------------------
// console_cmd.c
// ------------------------------------------------------------------------------------------------

#include "console_cmd.h"
#include "console.h"
#include "io.h"
#include "ipv4.h"
#include "pit.h"

// ------------------------------------------------------------------------------------------------
static void cmd_hello(const char* line)
{
    console_print("Hello World.\n");
}

// ------------------------------------------------------------------------------------------------
static void cmd_lsroute(const char* line)
{
    ipv4_print_route_table();
}

// ------------------------------------------------------------------------------------------------
static void cmd_reboot(const char* line)
{
    out8(0x64, 0xfe);   // Send reboot command to keyboard controller
}

// ------------------------------------------------------------------------------------------------
static void cmd_ticks(const char* line)
{
    console_print("%d\n", pit_ticks);
}

// ------------------------------------------------------------------------------------------------
ConsoleCmd console_cmd_table[] =
{
    { "hello", cmd_hello },
    { "lsroute", cmd_lsroute },
    { "reboot", cmd_reboot },
    { "ticks", cmd_ticks },
    { 0, 0 },
};
