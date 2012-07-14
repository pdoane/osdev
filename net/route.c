// ------------------------------------------------------------------------------------------------
// net/route.c
// ------------------------------------------------------------------------------------------------

#include "net/route.h"
#include "console/console.h"
#include "mem/vm.h"
#include "stdlib/string.h"

// ------------------------------------------------------------------------------------------------
static Link net_route_table = { &net_route_table, &net_route_table };

// ------------------------------------------------------------------------------------------------
const Net_Route* net_find_route(const IPv4_Addr* dst)
{
    Net_Route* route;
    list_for_each(route, net_route_table, link)
    {
        if ((dst->u.bits & route->mask.u.bits) == route->dst.u.bits)
        {
            return route;
        }
    }

    console_print("Failed to route IPv4 address\n");
    return 0;
}

// ------------------------------------------------------------------------------------------------
void net_add_route(const IPv4_Addr* dst, const IPv4_Addr* mask, const IPv4_Addr* gateway, Net_Intf* intf)
{
    Net_Route* route = vm_alloc(sizeof(Net_Route));
    link_init(&route->link);
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
    Net_Route* prev;
    list_for_each(prev, net_route_table, link)
    {
        if (prev->mask.u.bits > mask->u.bits)
        {
            break;
        }
    }

    link_after(&prev->link, &route->link);
}

// ------------------------------------------------------------------------------------------------
const IPv4_Addr* net_next_addr(const Net_Route* route, const IPv4_Addr* dst_addr)
{
    return route->gateway.u.bits ? &route->gateway : dst_addr;
}

// ------------------------------------------------------------------------------------------------
void net_print_route_table()
{
    console_print("%-15s  %-15s  %-15s  %s\n", "Destination", "Netmask", "Gateway", "Interface");

    Net_Route* route;
    list_for_each(route, net_route_table, link)
    {
        char dst_str[IPV4_ADDR_STRING_SIZE];
        char mask_str[IPV4_ADDR_STRING_SIZE];
        char gateway_str[IPV4_ADDR_STRING_SIZE];

        ipv4_addr_to_str(dst_str, sizeof(dst_str), &route->dst);
        ipv4_addr_to_str(mask_str, sizeof(mask_str), &route->mask);
        if (route->gateway.u.bits)
        {
            ipv4_addr_to_str(gateway_str, sizeof(gateway_str), &route->gateway);
        }
        else
        {
            strcpy(gateway_str, "On-link");
        }

        console_print("%-15s  %-15s  %-15s  %s\n", dst_str, mask_str, gateway_str, route->intf->name);
    }
}
