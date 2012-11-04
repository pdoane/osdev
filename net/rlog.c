// ------------------------------------------------------------------------------------------------
// net/rlog.c
// ------------------------------------------------------------------------------------------------

#include "net/rlog.h"
#include "net/buf.h"
#include "net/intf.h"
#include "net/port.h"
#include "net/udp.h"
#include "stdlib/link.h"
#include "stdlib/format.h"
#include "stdlib/stdarg.h"
#include "stdlib/string.h"

#include "console/console.h"

// ------------------------------------------------------------------------------------------------
void RlogPrint(const char *fmt, ...)
{
    char msg[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    ConsolePrint(msg);

    uint len = strlen(msg) + 1;

    // For each interface, broadcast a packet
    NetIntf *intf;
    ListForEach(intf, g_netIntfList, link)
    {
        if (!Ipv4AddrEq(&intf->broadcastAddr, &g_nullIpv4Addr))
        {
            NetBuf *pkt = NetAllocBuf();

            strcpy((char *)pkt->start, msg);
            pkt->end += len;

            UdpSend(&intf->broadcastAddr, PORT_OSHELPER, PORT_OSHELPER, pkt);
        }
    }
}
