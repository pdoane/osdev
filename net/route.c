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
    Link* it = net_route_table.next;
    Link* end = &net_route_table;

    while (it != end)
    {
        Net_Route* route = link_data(it, Net_Route, link);

        if ((dst->u.bits & route->mask.u.bits) == route->dst.u.bits)
        {
            return route;
        }

        it = it->next;
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
    Link* it = net_route_table.next;
    Link* end = &net_route_table;

    while (it != end)
    {
        Net_Route* it_route = link_data(it, Net_Route, link);

        if (it_route->mask.u.bits > mask->u.bits)
        {
            break;
        }

        it = it->next;
    }

    link_after(it, &route->link);
}

// ------------------------------------------------------------------------------------------------
void net_print_route_table()
{
    Link* it = net_route_table.next;
    Link* end = &net_route_table;

    console_print("%-15s  %-15s  %-15s  %s\n", "Destination", "Netmask", "Gateway", "Interface");
    while (it != end)
    {
        Net_Route* route = link_data(it, Net_Route, link);

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

        it = it->next;
    }
}
