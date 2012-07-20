// ------------------------------------------------------------------------------------------------
// net/ipv4.c
// ------------------------------------------------------------------------------------------------

#include "net/ipv4.h"
#include "net/checksum.h"
#include "net/eth.h"
#include "net/icmp.h"
#include "net/net.h"
#include "net/route.h"
#include "net/swap.h"
#include "net/tcp.h"
#include "net/udp.h"
#include "console/console.h"
#include "mem/mem_dump.h"
#include "mem/vm.h"

// ------------------------------------------------------------------------------------------------
void ipv4_rx(Net_Intf* intf, Net_Buf* pkt)
{
    ipv4_print(pkt);

    // Validate packet header
    if (pkt->start + sizeof(IPv4_Header) > pkt->end)
    {
        return;
    }

    const IPv4_Header* hdr = (const IPv4_Header*)pkt->start;

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

    // Update packet end
    u8* ip_end = pkt->start + net_swap16(hdr->len);
    if (ip_end > pkt->end)
    {
        console_print("IP Packet too long\n");
        return;
    }

    pkt->start += ihl << 2;
    pkt->end = ip_end;

    // Dispatch based on protocol
    switch (hdr->protocol)
    {
    case IP_PROTOCOL_ICMP:
        icmp_rx(intf, hdr, pkt);
        break;

    case IP_PROTOCOL_TCP:
        tcp_rx(intf, hdr, pkt);
        break;

    case IP_PROTOCOL_UDP:
        udp_rx(intf, hdr, pkt);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void ipv4_tx_intf(Net_Intf* intf, const IPv4_Addr* next_addr,
    const IPv4_Addr* dst_addr, u8 protocol, Net_Buf* pkt)
{
    // IPv4 Header
    pkt->start -= sizeof(IPv4_Header);

    IPv4_Header* hdr = (IPv4_Header*)pkt->start;
    hdr->ver_ihl = (4 << 4) | 5;
    hdr->tos = 0;
    hdr->len = net_swap16(pkt->end - pkt->start);
    hdr->id = net_swap16(0);
    hdr->offset = net_swap16(0);
    hdr->ttl = 64;
    hdr->protocol = protocol;
    hdr->checksum = 0;
    hdr->src = intf->ip_addr;
    hdr->dst = *dst_addr;

    uint checksum = net_checksum(pkt->start, pkt->start + sizeof(IPv4_Header));
    hdr->checksum = net_swap16(checksum);

    ipv4_print(pkt);

    intf->tx(intf, next_addr, ET_IPV4, pkt);
}

// ------------------------------------------------------------------------------------------------
void ipv4_tx(const IPv4_Addr* dst_addr, u8 protocol, Net_Buf* pkt)
{
    const Net_Route* route = net_find_route(dst_addr);

    if (route)
    {
        const IPv4_Addr* next_addr = net_next_addr(route, dst_addr);

        ipv4_tx_intf(route->intf, next_addr, dst_addr, protocol, pkt);
    }
}

// ------------------------------------------------------------------------------------------------
void ipv4_print(const Net_Buf* pkt)
{
    if (~net_trace & TRACE_NET)
    {
        return;
    }

    if (pkt->start + sizeof(IPv4_Header) > pkt->end)
    {
        return;
    }

    const IPv4_Header* hdr = (const IPv4_Header*)pkt->start;

    uint version = (hdr->ver_ihl >> 4) & 0xf;
    uint ihl = (hdr->ver_ihl) & 0xf;
    uint dscp = (hdr->tos >> 2) & 0x3f;
    uint ecn = (hdr->tos) & 0x3;
    u16 len = net_swap16(hdr->len);
    u16 id = net_swap16(hdr->id);
    u16 fragment = net_swap16(hdr->offset) & 0x1fff;
    u8 ttl = hdr->ttl;
    u8 protocol = hdr->protocol;
    u16 checksum = net_swap16(hdr->checksum);

    uint checksum2 = net_checksum(pkt->start, pkt->start + sizeof(IPv4_Header));

    char src_addr_str[IPV4_ADDR_STRING_SIZE];
    char dst_addr_str[IPV4_ADDR_STRING_SIZE];
    ipv4_addr_to_str(src_addr_str, sizeof(src_addr_str), &hdr->src);
    ipv4_addr_to_str(dst_addr_str, sizeof(dst_addr_str), &hdr->dst);

    console_print(" IPv4: version=%d ihl=%d dscp=%d ecn=%d\n",
            version, ihl, dscp, ecn);
    console_print(" IPv4: len=%d, id=%d, fragment=%d, ttl=%d, protocol=%d, checksum=%d%c\n",
            len, id, fragment, ttl, protocol, checksum,
            checksum2 ? '!' : ' ');
    console_print(" IPv4: dst=%s src=%s\n",
            dst_addr_str, src_addr_str);
}
