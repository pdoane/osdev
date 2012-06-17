// ------------------------------------------------------------------------------------------------
// ipv4.c
// ------------------------------------------------------------------------------------------------

#include "ipv4.h"
#include "console.h"
#include "eth.h"
#include "icmp.h"
#include "net.h"
#include "string.h"
#include "udp.h"
#include "vm.h"

// ------------------------------------------------------------------------------------------------
Link g_ipv4_route_table = { &g_ipv4_route_table, &g_ipv4_route_table };

// ------------------------------------------------------------------------------------------------
void ipv4_rx(Net_Intf* intf, const u8* pkt, uint len)
{
    ipv4_print(pkt, len);

    // Validate packet header
    if (len < sizeof(IPv4_Header))
    {
        return;
    }

    const IPv4_Header* hdr = (const IPv4_Header*)pkt;

    uint version = (hdr->ver_ihl >> 4) & 0xf;
    if (version != 4)
    {
        return;
    }

    // Fragments
    u16 fragment = net_swap16(hdr->offset) & 0x1fff;

    // Fragments are not handled yet
    if (fragment)
    {
        return;
    }

    // Jump to packet data
    uint ihl = (hdr->ver_ihl) & 0xf;
    const u8* data = pkt + (ihl << 2);
    uint data_len = len - (ihl << 2);

    // Dispatch based on protocol
    switch (hdr->protocol)
    {
    case IP_PROTOCOL_ICMP:
        icmp_rx(intf, pkt, len);  // Send the base IPv4 packet
        break;

    case IP_PROTOCOL_TCP:
        break;

    case IP_PROTOCOL_UDP:
        udp_rx(intf, data, data_len);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void ipv4_tx_intf(Net_Intf* intf, const IPv4_Addr* next_addr,
    const IPv4_Addr* dst_addr, u8 protocol, u8* pkt, uint len)
{
    // IPv4 Header
    pkt -= sizeof(IPv4_Header);
    len += sizeof(IPv4_Header);

    IPv4_Header* hdr = (IPv4_Header*)pkt;
    hdr->ver_ihl = (4 << 4) | 5;
    hdr->tos = 0;
    hdr->len = net_swap16(len);
    hdr->id = net_swap16(0);
    hdr->offset = net_swap16(0);
    hdr->ttl = 64;
    hdr->protocol = protocol;
    hdr->checksum = 0;
    hdr->src = intf->ip_addr;
    hdr->dst = *dst_addr;

    uint checksum = ipv4_checksum(pkt, sizeof(IPv4_Header));
    hdr->checksum = net_swap16(checksum);

    ipv4_print(pkt, len);

    intf->tx(intf, next_addr, ET_IPV4, pkt, len);
}

// ------------------------------------------------------------------------------------------------
void ipv4_tx(const IPv4_Addr* dst_addr, u8 protocol, u8* pkt, uint len)
{
    // Find an appropriate interface to route dst_addr
    const IPv4_Route* route = ipv4_find_route(dst_addr);
    const IPv4_Addr* next_addr = dst_addr;

    if (route)
    {
        // Use gateway if appropriate for the route
        if (route->gateway.u.bits)
        {
            next_addr = &route->gateway;
        }

        ipv4_tx_intf(route->intf, next_addr, dst_addr, protocol, pkt, len);
    }
}

// ------------------------------------------------------------------------------------------------
const IPv4_Route* ipv4_find_route(const IPv4_Addr* dst)
{
    Link* it = g_ipv4_route_table.next;
    Link* end = &g_ipv4_route_table;

    while (it != end)
    {
        IPv4_Route* route = link_data(it, IPv4_Route, link);

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
void ipv4_add_route(const IPv4_Addr* dst, const IPv4_Addr* mask, const IPv4_Addr* gateway, Net_Intf* intf)
{
    IPv4_Route* route = vm_alloc(sizeof(IPv4_Route));
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

    // Insert route at appropriate priority in the touble.
    Link* it = g_ipv4_route_table.next;
    Link* end = &g_ipv4_route_table;

    while (it != end)
    {
        IPv4_Route* it_route = link_data(it, IPv4_Route, link);

        if (it_route->mask.u.bits > mask->u.bits)
        {
            break;
        }

        it = it->next;
    }

    link_after(it, &route->link);
}

// ------------------------------------------------------------------------------------------------
void ipv4_print_route_table()
{
    Link* it = g_ipv4_route_table.next;
    Link* end = &g_ipv4_route_table;

    console_print("%-15s  %-15s  %-15s  %s\n", "Destination", "Netmask", "Gateway", "Interface");
    while (it != end)
    {
        IPv4_Route* route = link_data(it, IPv4_Route, link);

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

// ------------------------------------------------------------------------------------------------
u16 ipv4_checksum(const u8* data, uint len)
{
    uint sum = 0;
    u16* p = (u16*)data;

    while (len > 1)
    {
        sum += *p++;
        len -= 2;
    }

    if (len)
    {
        sum += *(u8*)p;
    }

    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    u16 temp = ~sum;
    return ((temp & 0x00ff) << 8) | ((temp & 0xff00) >> 8);
}

// ------------------------------------------------------------------------------------------------
void ipv4_print(const u8* pkt, uint len)
{
    if (!net_trace)
    {
        return;
    }

    if (len < sizeof(IPv4_Header))
    {
        return;
    }

    const IPv4_Header* hdr = (const IPv4_Header*)pkt;

    uint version = (hdr->ver_ihl >> 4) & 0xf;
    uint ihl = (hdr->ver_ihl) & 0xf;
    uint dscp = (hdr->tos >> 2) & 0x3f;
    uint ecn = (hdr->tos) & 0x3;
    u16 packet_len = net_swap16(hdr->len);
    u16 id = net_swap16(hdr->id);
    u16 fragment = net_swap16(hdr->offset) & 0x1fff;
    u8 ttl = hdr->ttl;
    u8 protocol = hdr->protocol;
    u16 checksum = net_swap16(hdr->checksum);

    uint checksum2 = ipv4_checksum(pkt, sizeof(IPv4_Header));

    char src_addr_str[IPV4_ADDR_STRING_SIZE];
    char dst_addr_str[IPV4_ADDR_STRING_SIZE];
    ipv4_addr_to_str(src_addr_str, sizeof(src_addr_str), &hdr->src);
    ipv4_addr_to_str(dst_addr_str, sizeof(dst_addr_str), &hdr->dst);

    console_print(" IPv4: version=%d ihl=%d dscp=%d ecn=%d\n",
            version, ihl, dscp, ecn);
    console_print(" IPv4: len=%d, id=%d, fragment=%d, ttl=%d, protocol=%d, checksum=%d%c\n",
            packet_len, id, fragment, ttl, protocol, checksum,
            checksum2 ? '!' : ' ');
    console_print(" IPv4: dst=%s src=%s\n",
            dst_addr_str, src_addr_str);
}
