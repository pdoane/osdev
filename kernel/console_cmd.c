// ------------------------------------------------------------------------------------------------
// console_cmd.c
// ------------------------------------------------------------------------------------------------

#include "console_cmd.h"
#include "console.h"
#include "dns.h"
#include "icmp.h"
#include "io.h"
#include "ipv4.h"
#include "ntp.h"
#include "pit.h"
#include "rtc.h"

// ------------------------------------------------------------------------------------------------
static void cmd_datetime(uint argc, const char** argv)
{
    char buf[TIME_STRING_SIZE];

    DateTime dt;
    rtc_get_time(&dt);
    format_time(buf, sizeof(buf), &dt);

    console_print("%s\n", buf);
}

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
static void cmd_host(uint argc, const char** argv)
{
    if (argc != 2)
    {
        console_print("Usage: host <host name>\n");
        return;
    }

    const char* host_name = argv[1];

    dns_query_host(host_name, 0);
}

// ------------------------------------------------------------------------------------------------
static void cmd_lsroute(uint argc, const char** argv)
{
    ipv4_print_route_table();
}

// ------------------------------------------------------------------------------------------------
static void cmd_ping(uint argc, const char** argv)
{
    if (argc != 2)
    {
        console_print("Usage: ping <dest ipv4 address>\n");
        return;
    }

    IPv4_Addr dst_addr;
    if (!str_to_ipv4_addr(&dst_addr, argv[1]))
    {
        console_print("Failed to parse destination address\n");
        return;
    }

    icmp_echo_request(&dst_addr, 1, 2, 0, 0);
}

// ------------------------------------------------------------------------------------------------
static void cmd_synctime(uint argc, const char** argv)
{
    if (argc != 2)
    {
        console_print("Usage: synctime <dest ipv4 address>\n");
        return;
    }

    IPv4_Addr dst_addr;
    if (!str_to_ipv4_addr(&dst_addr, argv[1]))
    {
        console_print("Failed to parse destination address\n");
        return;
    }

    ntp_tx(&dst_addr);
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
    { "datetime", cmd_datetime },
    { "echo", cmd_echo },
    { "hello", cmd_hello },
    { "help", cmd_help },
    { "host", cmd_host },
    { "lsroute", cmd_lsroute },
    { "ping", cmd_ping },
    { "reboot", cmd_reboot },
    { "synctime", cmd_synctime },
    { "ticks", cmd_ticks },
    { 0, 0 },
};
