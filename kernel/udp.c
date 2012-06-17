// ------------------------------------------------------------------------------------------------
// udp.c
// ------------------------------------------------------------------------------------------------

#include "udp.h"
#include "console.h"
#include "dns.h"
#include "ipv4.h"
#include "net.h"
#include "net_port.h"

// ------------------------------------------------------------------------------------------------
void udp_rx(Net_Intf* intf, const u8* pkt, uint len)
{
    udp_print(pkt, len);

    // Validate packet header
    if (len < sizeof(UDP_Header))
    {
        return;
    }

    const UDP_Header* hdr = (const UDP_Header*)pkt;

    u16 src_port = net_swap16(hdr->src_port);
    //u16 dst_port = net_swap16(hdr->dst_port);

    const u8* data = pkt + sizeof(UDP_Header);
    uint data_len = len - sizeof(UDP_Header);

    switch (src_port)
    {
    case PORT_DNS:
        dns_rx(intf, data, data_len);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void udp_tx(const IPv4_Addr* dst_addr, uint dst_port, uint src_port, u8* pkt, uint len)
{
    // UDP Header
    pkt -= sizeof(UDP_Header);
    len += sizeof(UDP_Header);

    UDP_Header* hdr = (UDP_Header*)pkt;
    hdr->src_port = net_swap16(src_port);
    hdr->dst_port = net_swap16(dst_port);
    hdr->len = net_swap16(len);
    hdr->checksum = 0;  // don't compute checksum yet

    udp_print(pkt, len);

    ipv4_tx(dst_addr, IP_PROTOCOL_UDP, pkt, len);
}

// ------------------------------------------------------------------------------------------------
void udp_print(const u8* pkt, uint len)
{
    if (!net_trace)
    {
        return;
    }

    if (len < sizeof(UDP_Header))
    {
        return;
    }

    const UDP_Header* hdr = (const UDP_Header*)pkt;

    u16 src_port = net_swap16(hdr->src_port);
    u16 dst_port = net_swap16(hdr->dst_port);
    u16 packet_len = net_swap16(hdr->len);
    u16 checksum = net_swap16(hdr->checksum);

    console_print("  UDP: src=%d dst=%d len=%d checksum=%d%c\n",
        src_port, dst_port, packet_len, checksum);
}
