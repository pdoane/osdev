// ------------------------------------------------------------------------------------------------
// net/loopback.c
// ------------------------------------------------------------------------------------------------

#include "net/loopback.h"
#include "net/arp.h"
#include "net/eth.h"
#include "net/ipv4.h"
#include "net/ipv6.h"
#include "net/intf.h"
#include "net/route.h"

// ------------------------------------------------------------------------------------------------
static void LoopPoll(NetIntf *intf)
{
}

// ------------------------------------------------------------------------------------------------
static void LoopSend(NetIntf *intf, const void *dstAddr, u16 etherType, NetBuf *pkt)
{
    // Route packet by protocol
    switch (etherType)
    {
    case ET_ARP:
        ArpRecv(intf, pkt);
        break;

    case ET_IPV4:
        Ipv4Recv(intf, pkt);
        break;

    case ET_IPV6:
        Ipv6Recv(intf, pkt);
        break;
    }

    NetReleaseBuf(pkt);
}

// ------------------------------------------------------------------------------------------------
void LoopbackInit()
{
    Ipv4Addr ipAddr = { { { 127, 0, 0, 1 } } };
    Ipv4Addr subnetMask = { { { 255, 255, 255, 255 } } };

    // Create net interface
    NetIntf *intf = NetIntfCreate();
    intf->ethAddr = g_nullEthAddr;
    intf->ipAddr = ipAddr;
    intf->name = "loop";
    intf->poll = LoopPoll;
    intf->send = LoopSend;
    intf->devSend = 0;

    NetIntfAdd(intf);

    // Add routing entries
    NetAddRoute(&ipAddr, &subnetMask, 0, intf);
}
