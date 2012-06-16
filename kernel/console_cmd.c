// ------------------------------------------------------------------------------------------------
// console_cmd.c
// ------------------------------------------------------------------------------------------------

#include "console_cmd.h"
#include "console.h"
#include "io.h"
#include "ipv4.h"
#include "pit.h"

// ------------------------------------------------------------------------------------------------
static void cmd_echo(uint argc, const char** argv)
{
    for (uint i = 0; i < argc; ++i)
    {
        console_print("%s\n", argv[i]);
    }
}

// ------------------------------------------------------------------------------------------------
static void cmd_hello(uint argc, const char** argv)
{
    console_print("Hello World.\n");
}

// ------------------------------------------------------------------------------------------------
static void cmd_help(uint argc, const char** argv)
{
    ConsoleCmd* cmd = console_cmd_table;
    while (cmd->name)
    {
        console_print("%s\n", cmd->name);

        ++cmd;
    }
}

// ------------------------------------------------------------------------------------------------
static void cmd_lsroute(uint argc, const char** argv)
{
    ipv4_print_route_table();
}

// ------------------------------------------------------------------------------------------------
static void cmd_reboot(uint argc, const char** argv)
{
    out8(0x64, 0xfe);   // Send reboot command to keyboard controller
}

// ------------------------------------------------------------------------------------------------
static void cmd_ticks(uint argc, const char** argv)
{
    console_print("%d\n", pit_ticks);
}

// ------------------------------------------------------------------------------------------------
ConsoleCmd console_cmd_table[] =
{
    { "echo", cmd_echo },
    { "hello", cmd_hello },
    { "help", cmd_help },
    { "lsroute", cmd_lsroute },
    { "reboot", cmd_reboot },
    { "ticks", cmd_ticks },
    { 0, 0 },
};
