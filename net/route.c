// ------------------------------------------------------------------------------------------------
// net/route.c
// ------------------------------------------------------------------------------------------------

#include "net/route.h"
#include "console/console.h"
#include "mem/vm.h"
#include "stdlib/string.h"

// ------------------------------------------------------------------------------------------------
static Link s_routeTable = { &s_routeTable, &s_routeTable };

// ------------------------------------------------------------------------------------------------
const NetRoute *NetFindRoute(const Ipv4Addr *dst)
{
    NetRoute *route;
    ListForEach(route, s_routeTable, link)
    {
        if ((dst->u.bits & route->mask.u.bits) == route->dst.u.bits)
        {
            return route;
        }
    }

    ConsolePrint("Failed to route IPv4 address\n");
    return 0;
}

// ------------------------------------------------------------------------------------------------
void NetAddRoute(const Ipv4Addr *dst, const Ipv4Addr *mask, const Ipv4Addr *gateway, NetIntf *intf)
{
    NetRoute *route = VMAlloc(sizeof(NetRoute));
    LinkInit(&route->link);
    route->dst = *dst;
    route->mask = *mask;
    if (gateway)
    {
        route->gateway = *gateway;
    }
    else
    {
        route->gateway.u.bits = 0;
    }

    route->intf = intf;

    // Insert route at appropriate priority in the table.
    NetRoute *prev;
    ListForEach(prev, s_routeTable, link)
    {
        if (prev->mask.u.bits > mask->u.bits)
        {
            break;
        }
    }

    LinkAfter(&prev->link, &route->link);
}

// ------------------------------------------------------------------------------------------------
const Ipv4Addr *NetNextAddr(const NetRoute *route, const Ipv4Addr *dstAddr)
{
    return route->gateway.u.bits ? &route->gateway : dstAddr;
}

// ------------------------------------------------------------------------------------------------
void NetPrintRouteTable()
{
    ConsolePrint("%-15s  %-15s  %-15s  %s\n", "Destination", "Netmask", "Gateway", "Interface");

    NetRoute *route;
    ListForEach(route, s_routeTable, link)
    {
        char dstStr[IPV4_ADDR_STRING_SIZE];
        char maskStr[IPV4_ADDR_STRING_SIZE];
        char gatewayStr[IPV4_ADDR_STRING_SIZE];

        Ipv4AddrToStr(dstStr, sizeof(dstStr), &route->dst);
        Ipv4AddrToStr(maskStr, sizeof(maskStr), &route->mask);
        if (route->gateway.u.bits)
        {
            Ipv4AddrToStr(gatewayStr, sizeof(gatewayStr), &route->gateway);
        }
        else
        {
            strcpy(gatewayStr, "On-link");
        }

        ConsolePrint("%-15s  %-15s  %-15s  %s\n", dstStr, maskStr, gatewayStr, route->intf->name);
    }
}
