// ------------------------------------------------------------------------------------------------
// net/net.c
// ------------------------------------------------------------------------------------------------

#include "net/net.h"
#include "net/arp.h"
#include "net/dhcp.h"
#include "net/loopback.h"
#include "net/tcp.h"

// ------------------------------------------------------------------------------------------------
// Globals

u8 g_netTrace = 0;

// ------------------------------------------------------------------------------------------------
void NetInit()
{
    LoopbackInit();
    ArpInit();
    TcpInit();

    // Initialize interfaces
    NetIntf *intf;
    ListForEach(intf, g_netIntfList, link)
    {
        // Check if interface needs IP address dynamically assigned
        if (!intf->ipAddr.u.bits)
        {
            DhcpDiscover(intf);
        }
    }
}

// ------------------------------------------------------------------------------------------------
void NetPoll()
{
    // Poll interfaces
    NetIntf *intf;
    ListForEach(intf, g_netIntfList, link)
    {
        intf->poll(intf);
    }

    TcpPoll();
}
