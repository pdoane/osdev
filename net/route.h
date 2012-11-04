// ------------------------------------------------------------------------------------------------
// net/route.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/intf.h"

// ------------------------------------------------------------------------------------------------
// Route Entry

typedef struct NetRoute
{
    Link link;
    Ipv4Addr dst;
    Ipv4Addr mask;
    Ipv4Addr gateway;
    NetIntf *intf;
} NetRoute;

// ------------------------------------------------------------------------------------------------
// Functions

const NetRoute *NetFindRoute(const Ipv4Addr *dst);
void NetAddRoute(const Ipv4Addr *dst, const Ipv4Addr *mask, const Ipv4Addr *gateway, NetIntf *intf);
const Ipv4Addr *NetNextAddr(const NetRoute *route, const Ipv4Addr *dstAddr);
void NetPrintRouteTable();
