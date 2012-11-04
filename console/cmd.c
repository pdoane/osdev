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
static void CmdDateTime(uint argc, const char **argv)
{
    char buf[TIME_STRING_SIZE];

    DateTime dt;
    RtcGetTime(&dt);
    FormatTime(buf, sizeof(buf), &dt);

    ConsolePrint("%s\n", buf);
}

// ------------------------------------------------------------------------------------------------
static void CmdDetect(uint argc, const char **argv)
{
    CpuDetect();
}

// ------------------------------------------------------------------------------------------------
static void CmdEcho(uint argc, const char **argv)
{
    for (uint i = 0; i < argc; ++i)
    {
        ConsolePrint("%s\n", argv[i]);
    }
}

// ------------------------------------------------------------------------------------------------
static void CmdGfx(uint argc, const char **argv)
{
    ConsolePrint("Starting 3D graphics...\n");
    GfxStart();
}

// ------------------------------------------------------------------------------------------------
static void CmdHello(uint argc, const char **argv)
{
    ConsolePrint("Hello World.\n");
}

// ------------------------------------------------------------------------------------------------
static void CmdHelp(uint argc, const char **argv)
{
    const ConsoleCmd *cmd = g_consoleCmdTable;
    while (cmd->name)
    {
        ConsolePrint("%s\n", cmd->name);

        ++cmd;
    }
}

// ------------------------------------------------------------------------------------------------
static void HttpOnTcpState(TcpConn *conn, uint oldState, uint newState)
{
    const char *msg = (const char *)conn->ctx;

    if (newState == TCP_ESTABLISHED)
    {
        TcpSend(conn, msg, strlen(msg));
    }

    if (newState == TCP_CLOSE_WAIT)
    {
        TcpClose(conn);
    }
}

static void HttpOnTcpData(TcpConn *conn, const u8 *data, uint len)
{
    uint col = 0;

    for (uint i = 0; i < len; ++i)
    {
        char c = data[i];
        ConsolePutChar(c);
        ++col;
        if (c == '\n')
        {
            col = 0;
        }
        else if (col == 80)
        {
            ConsolePutChar('\n');
            col = 0;
        }
    }
}

static void CmdHttp(uint argc, const char **argv)
{
    if (argc != 3)
    {
        ConsolePrint("Usage: http <dest ipv4 address> <path>\n");
        return;
    }

    Ipv4Addr dstAddr;
    if (!StrToIpv4Addr(&dstAddr, argv[1]))
    {
        ConsolePrint("Failed to parse destination address\n");
        return;
    }

    u16 port = 80;
    static char buf[256];

    snprintf(buf, sizeof(buf), "GET %s HTTP/1.0\r\n\r\n", argv[2]);

    TcpConn *conn = TcpCreate();
    conn->ctx = buf;
    conn->onState = HttpOnTcpState;
    conn->onData = HttpOnTcpData;

    TcpConnect(conn, &dstAddr, port);
}

// ------------------------------------------------------------------------------------------------
static void CmdHost(uint argc, const char **argv)
{
    if (argc != 2)
    {
        ConsolePrint("Usage: host <host name>\n");
        return;
    }

    const char *hostName = argv[1];

    DnsQueryHost(hostName, 0);
}

// ------------------------------------------------------------------------------------------------
static void CmdLsConn(uint argc, const char **argv)
{
    ConsolePrint("%-21s  %-21s  %s\n", "Local Address", "Remote Address", "State");

    TcpConn *conn;
    ListForEach(conn, g_tcpActiveConns, link)
    {
        char localStr[IPV4_ADDR_PORT_STRING_SIZE];
        char remoteStr[IPV4_ADDR_PORT_STRING_SIZE];
        const char *stateStr = "Unknown";

        Ipv4AddrPortToStr(localStr, sizeof(localStr), &conn->localAddr, conn->localPort);
        Ipv4AddrPortToStr(remoteStr, sizeof(remoteStr), &conn->remoteAddr, conn->remotePort);

        if (conn->state <= TCP_TIME_WAIT)
        {
            stateStr = g_tcpStateStrs[conn->state];
        }

        ConsolePrint("%-21s  %-21s  %s\n", localStr, remoteStr, stateStr);
    }
}

// ------------------------------------------------------------------------------------------------
static void CmdLsRoute(uint argc, const char **argv)
{
    NetPrintRouteTable();
}

// ------------------------------------------------------------------------------------------------
static void CmdMem(uint argc, const char **argv)
{
    ConsolePrint("net buf: %d\n", g_netBufAllocCount);
}

// ------------------------------------------------------------------------------------------------
static void CmdNetTrace(uint argc, const char **argv)
{
    if (argc != 2)
    {
        ConsolePrint("Usage: net_trace <level>\n");
        return;
    }

    uint level;
    if (sscanf(argv[1], "%d", &level) == 1)
    {
        g_netTrace = level;
    }
}

// ------------------------------------------------------------------------------------------------
static void CmdPing(uint argc, const char **argv)
{
    if (argc != 2)
    {
        ConsolePrint("Usage: ping <dest ipv4 address>\n");
        return;
    }

    Ipv4Addr dstAddr;
    if (!StrToIpv4Addr(&dstAddr, argv[1]))
    {
        ConsolePrint("Failed to parse destination address\n");
        return;
    }

    IcmpEchoRequest(&dstAddr, 1, 2, 0, 0);
}

// ------------------------------------------------------------------------------------------------
static void CmdSyncTime(uint argc, const char **argv)
{
    if (argc != 2)
    {
        ConsolePrint("Usage: synctime <dest ipv4 address>\n");
        return;
    }

    Ipv4Addr dstAddr;
    if (!StrToIpv4Addr(&dstAddr, argv[1]))
    {
        ConsolePrint("Failed to parse destination address\n");
        return;
    }

    NtpSend(&dstAddr);
}

// ------------------------------------------------------------------------------------------------
static void CmdReboot(uint argc, const char **argv)
{
    IoWrite8(0x64, 0xfe);   // Send reboot command to keyboard controller
}

// ------------------------------------------------------------------------------------------------
static void CmdRlog(uint argc, const char **argv)
{
    if (argc != 2)
    {
        ConsolePrint("Usage: rlog <message>\n");
        return;
    }

    const char *msg = argv[1];
    RlogPrint("%s", msg);
}

// ------------------------------------------------------------------------------------------------
static void CmdTicks(uint argc, const char **argv)
{
    ConsolePrint("%d\n", g_pitTicks);
}

// ------------------------------------------------------------------------------------------------
static void CmdPeek(uint argc, const char **argv)
{
    if (argc != 2)
    {
        ConsolePrint("Usage: peek <address>\n");
        return;
    }

    volatile u32 *addr = (u32 *)(strtoul(argv[1], 0, 0));
    RlogPrint("Value of %p is 0x%X", addr, *addr);
}

// ------------------------------------------------------------------------------------------------
static void CmdPoke(uint argc, const char **argv)
{
    if (argc != 3)
    {
        ConsolePrint("Usage: poke <address> <value>\n");
        return;
    }

    volatile u32 *addr = (u32 *)(strtoul(argv[1], 0, 0));
    u32 value    = (u32)(strtoul(argv[2], 0, 0));
    RlogPrint("Writing to %p: 0x%X", addr, value);
    *addr = value;
}

// ------------------------------------------------------------------------------------------------
const ConsoleCmd g_consoleCmdTable[] =
{
    { "datetime", CmdDateTime },
    { "detect", CmdDetect },
    { "echo", CmdEcho },
    { "gfx", CmdGfx },
    { "hello", CmdHello },
    { "help", CmdHelp },
    { "host", CmdHost },
    { "http", CmdHttp },
    { "lsconn", CmdLsConn },
    { "lsroute", CmdLsRoute },
    { "mem", CmdMem },
    { "net_trace", CmdNetTrace },
    { "ping", CmdPing },
    { "reboot", CmdReboot },
    { "rlog", CmdRlog },
    { "synctime", CmdSyncTime },
    { "ticks", CmdTicks },
    { "peek", CmdPeek },
    { "poke", CmdPoke },
    { 0, 0 },
};
