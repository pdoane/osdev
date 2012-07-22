// ------------------------------------------------------------------------------------------------
// console/cmd.c
// ------------------------------------------------------------------------------------------------

#include "console/cmd.h"
#include "console/console.h"
#include "cpu/detect.h"
#include "cpu/io.h"
#include "gfx/gfx.h"
#include "net/dns.h"
#include "net/icmp.h"
#include "net/ipv4.h"
#include "net/net.h"
#include "net/ntp.h"
#include "net/port.h"
#include "net/rlog.h"
#include "net/route.h"
#include "net/tcp.h"
#include "net/udp.h"
#include "stdlib/format.h"
#include "stdlib/string.h"
#include "time/pit.h"
#include "time/rtc.h"

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
static void cmd_detect(uint argc, const char** argv)
{
    cpu_detect();
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
static void cmd_gfx(uint argc, const char** argv)
{
    console_print("Starting 3D graphics...\n");
    gfx_start();
}

// ------------------------------------------------------------------------------------------------
static void cmd_hello(uint argc, const char** argv)
{
    console_print("Hello World.\n");
}

// ------------------------------------------------------------------------------------------------
static void cmd_help(uint argc, const char** argv)
{
    const ConsoleCmd* cmd = console_cmd_table;
    while (cmd->name)
    {
        console_print("%s\n", cmd->name);

        ++cmd;
    }
}

// ------------------------------------------------------------------------------------------------
static void tcp_state(TCP_Conn* conn, uint old_state, uint new_state)
{
    const char* msg = (const char*)conn->ctx;

    if (new_state == TCP_ESTABLISHED)
    {
        tcp_send(conn, msg, strlen(msg));
    }

    if (new_state == TCP_CLOSE_WAIT)
    {
        tcp_close(conn);
    }
}

static void tcp_data(TCP_Conn* conn, const u8* data, uint len)
{
    uint col = 0;

    for (uint i = 0; i < len; ++i)
    {
        char c = data[i];
        console_putchar(c);
        ++col;
        if (c == '\n')
        {
            col = 0;
        }
        else if (col == 80)
        {
            console_putchar('\n');
            col = 0;
        }
    }
}

static void cmd_http(uint argc, const char** argv)
{
    if (argc != 3)
    {
        console_print("Usage: http <dest ipv4 address> <path>\n");
        return;
    }

    IPv4_Addr dst_addr;
    if (!str_to_ipv4_addr(&dst_addr, argv[1]))
    {
        console_print("Failed to parse destination address\n");
        return;
    }

    u16 port = 80;
    static char buf[256];

    snprintf(buf, sizeof(buf), "GET %s HTTP/1.0\r\n\r\n", argv[2]);

    TCP_Conn* conn = tcp_create();
    conn->ctx = buf;
    conn->on_state = tcp_state;
    conn->on_data = tcp_data;

    tcp_connect(conn, &dst_addr, port);
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
static void cmd_lsconn(uint argc, const char** argv)
{
    console_print("%-21s  %-21s  %s\n", "Local Address", "Remote Address", "State");

    TCP_Conn* conn;
    list_for_each(conn, tcp_active_conns, link)
    {
        char local_str[IPV4_ADDR_PORT_STRING_SIZE];
        char remote_str[IPV4_ADDR_PORT_STRING_SIZE];
        const char* state_str = "Unknown";

        ipv4_addr_port_to_str(local_str, sizeof(local_str), &conn->local_addr, conn->local_port);
        ipv4_addr_port_to_str(remote_str, sizeof(remote_str), &conn->remote_addr, conn->remote_port);

        if (conn->state <= TCP_TIME_WAIT)
        {
            state_str = tcp_state_strs[conn->state];
        }

        console_print("%-21s  %-21s  %s\n", local_str, remote_str, state_str);
    }
}

// ------------------------------------------------------------------------------------------------
static void cmd_lsroute(uint argc, const char** argv)
{
    net_print_route_table();
}

// ------------------------------------------------------------------------------------------------
static void cmd_mem(uint argc, const char** argv)
{
    console_print("net buf: %d\n", net_buf_alloc_count);
}

// ------------------------------------------------------------------------------------------------
static void cmd_net_trace(uint argc, const char** argv)
{
    if (argc != 2)
    {
        console_print("Usage: net_trace <level>\n");
        return;
    }

    uint level;
    if (sscanf(argv[1], "%d", &level) == 1)
    {
        net_trace = level;
    }
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
static void cmd_rlog(uint argc, const char** argv)
{
    if (argc != 2)
    {
        console_print("Usage: rlog <message>\n");
        return;
    }

    const char* msg = argv[1];
    rlog_print("%s", msg);
}

// ------------------------------------------------------------------------------------------------
static void cmd_ticks(uint argc, const char** argv)
{
    console_print("%d\n", pit_ticks);
}

// ------------------------------------------------------------------------------------------------
const ConsoleCmd console_cmd_table[] =
{
    { "datetime", cmd_datetime },
    { "detect", cmd_detect },
    { "echo", cmd_echo },
    { "gfx", cmd_gfx },
    { "hello", cmd_hello },
    { "help", cmd_help },
    { "host", cmd_host },
    { "http", cmd_http },
    { "lsconn", cmd_lsconn },
    { "lsroute", cmd_lsroute },
    { "mem", cmd_mem },
    { "net_trace", cmd_net_trace },
    { "ping", cmd_ping },
    { "reboot", cmd_reboot },
    { "rlog", cmd_rlog },
    { "synctime", cmd_synctime },
    { "ticks", cmd_ticks },
    { 0, 0 },
};
