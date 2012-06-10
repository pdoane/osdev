// ------------------------------------------------------------------------------------------------
// ipv4.c
// ------------------------------------------------------------------------------------------------

#include "ipv4.h"
#include "console.h"
#include "eth.h"
#include "icmp.h"
#include "net.h"
#include "net_config.h"

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
    //u8* data = pkt + (ihl << 2);

    // Dispatch based on protocol
    switch (hdr->protocol)
    {
    case IP_PROTOCOL_ICMP:
        icmp_rx(intf, pkt, len);  // Send the base IPv4 packet
        break;

    case IP_PROTOCOL_TCP:
        break;

    case IP_PROTOCOL_UDP:
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void ipv4_tx(Net_Intf* intf, const IPv4_Addr* dst_addr, u8 protocol, u8* buf, uint len)
{
    uint ip_packet_size = len - sizeof(Eth_Header);

    // IPv4 Header
    u8* pkt = buf + sizeof(Eth_Header);
    IPv4_Header* hdr = (IPv4_Header*)pkt;
    hdr->ver_ihl = (4 << 4) | 5;
    hdr->tos = 0;
    hdr->len = net_swap16(ip_packet_size);
    hdr->id = net_swap16(0);
    hdr->offset = net_swap16(0);
    hdr->ttl = 64;
    hdr->protocol = protocol;
    hdr->checksum = 0;
    hdr->src = intf->ip_addr;
    hdr->dst = *dst_addr;

    uint checksum = ipv4_checksum(pkt, sizeof(IPv4_Header));
    hdr->checksum = net_swap16(checksum);

    ipv4_print(pkt, ip_packet_size);

    eth_tx_ipv4(intf, dst_addr, buf, len);
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

    char src_addr_str[IPV4_ADDR_STRING_SIZE];
    char dst_addr_str[IPV4_ADDR_STRING_SIZE];
    ipv4_addr_to_str(src_addr_str, sizeof(src_addr_str), &hdr->src);
    ipv4_addr_to_str(dst_addr_str, sizeof(dst_addr_str), &hdr->dst);

    console_print(" IPv4: version=%d ihl=%d dscp=%d ecn=%d\n",
            version, ihl, dscp, ecn);
    console_print(" IPv4: len=%d, id=%d, fragment=%d, ttl=%d, protocol=%d, checksum=%d\n",
            packet_len, id, fragment, ttl, protocol, checksum);
    console_print(" IPv4: dst=%s src=%s\n",
            dst_addr_str, src_addr_str);
}
