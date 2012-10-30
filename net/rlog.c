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
void rlog_print(const char* fmt, ...)
{
    char msg[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    console_print(msg);

    uint len = strlen(msg) + 1;

    // For each interface, broadcast a packet
    Net_Intf* intf;
    list_for_each(intf, net_intf_list, link)
    {
        if (!ipv4_addr_eq(&intf->broadcast_addr, &null_ipv4_addr))
        {
            Net_Buf* pkt = net_alloc_buf();

            strcpy((char*)pkt->start, msg);
            pkt->end += len;

            udp_tx(&intf->broadcast_addr, PORT_OSHELPER, PORT_OSHELPER, pkt);
        }
    }
}
