// ------------------------------------------------------------------------------------------------
// net/udp.c
// ------------------------------------------------------------------------------------------------

#include "net/udp.h"
#include "net/dhcp.h"
#include "net/dns.h"
#include "net/ipv4.h"
#include "net/net.h"
#include "net/ntp.h"
#include "net/port.h"
#include "net/swap.h"
#include "console/console.h"

// ------------------------------------------------------------------------------------------------
void udp_rx(Net_Intf* intf, u8* pkt, u8* end)
{
    udp_print(pkt, end);

    // Validate packet header
    if (pkt + sizeof(UDP_Header) > end)
    {
        return;
    }

    const UDP_Header* hdr = (const UDP_Header*)pkt;

    u16 src_port = net_swap16(hdr->src_port);
    //u16 dst_port = net_swap16(hdr->dst_port);

    const u8* data = pkt + sizeof(UDP_Header);

    switch (src_port)
    {
    case PORT_DNS:
        dns_rx(intf, data, end);
        break;

    case PORT_BOOTP_SERVER:
        dhcp_rx(intf, data, end);
        break;

    case PORT_NTP:
        ntp_rx(intf, data, end);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void udp_tx(const IPv4_Addr* dst_addr, uint dst_port, uint src_port, u8* pkt, u8* end)
{
    // UDP Header
    pkt -= sizeof(UDP_Header);

    UDP_Header* hdr = (UDP_Header*)pkt;
    hdr->src_port = net_swap16(src_port);
    hdr->dst_port = net_swap16(dst_port);
    hdr->len = net_swap16(end - pkt);
    hdr->checksum = 0;  // don't compute checksum yet

    udp_print(pkt, end);

    ipv4_tx(dst_addr, IP_PROTOCOL_UDP, pkt, end);
}

// ------------------------------------------------------------------------------------------------
void udp_tx_intf(Net_Intf* intf, const IPv4_Addr* dst_addr, uint dst_port, uint src_port, u8* pkt, u8* end)
{
    // UDP Header
    pkt -= sizeof(UDP_Header);

    UDP_Header* hdr = (UDP_Header*)pkt;
    hdr->src_port = net_swap16(src_port);
    hdr->dst_port = net_swap16(dst_port);
    hdr->len = net_swap16(end - pkt);
    hdr->checksum = 0;  // don't compute checksum yet

    udp_print(pkt, end);

    ipv4_tx_intf(intf, dst_addr, dst_addr, IP_PROTOCOL_UDP, pkt, end);
}

// ------------------------------------------------------------------------------------------------
void udp_print(const u8* pkt, const u8* end)
{
    if (~net_trace & (1 << 2))
    {
        return;
    }

    if (pkt + sizeof(UDP_Header) > end)
    {
        return;
    }

    const UDP_Header* hdr = (const UDP_Header*)pkt;

    u16 src_port = net_swap16(hdr->src_port);
    u16 dst_port = net_swap16(hdr->dst_port);
    u16 len = net_swap16(hdr->len);
    u16 checksum = net_swap16(hdr->checksum);

    console_print("  UDP: src=%d dst=%d len=%d checksum=%d\n",
        src_port, dst_port, len, checksum);
}
