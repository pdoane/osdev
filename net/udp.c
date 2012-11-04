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
void UdpRecv(NetIntf *intf, const Ipv4Header *ipHdr, NetBuf *pkt)
{
    UdpPrint(pkt);

    // Validate packet header
    if (pkt->start + sizeof(UdpHeader) > pkt->end)
    {
        return;
    }

    const UdpHeader *hdr = (const UdpHeader *)pkt->start;

    u16 srcPort = NetSwap16(hdr->srcPort);
    //u16 dstPort = NetSwap16(hdr->dstPort);

    pkt->start += sizeof(UdpHeader);

    switch (srcPort)
    {
    case PORT_DNS:
        DnsRecv(intf, pkt);
        break;

    case PORT_BOOTP_SERVER:
        DhcpRecv(intf, pkt);
        break;

    case PORT_NTP:
        NtpRecv(intf, pkt);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void UdpSend(const Ipv4Addr *dstAddr, uint dstPort, uint srcPort, NetBuf *pkt)
{
    // UDP Header
    pkt->start -= sizeof(UdpHeader);

    UdpHeader *hdr = (UdpHeader *)pkt->start;
    hdr->srcPort = NetSwap16(srcPort);
    hdr->dstPort = NetSwap16(dstPort);
    hdr->len = NetSwap16(pkt->end - pkt->start);
    hdr->checksum = 0;  // don't compute checksum yet

    UdpPrint(pkt);

    Ipv4Send(dstAddr, IP_PROTOCOL_UDP, pkt);
}

// ------------------------------------------------------------------------------------------------
void UdpSendIntf(NetIntf *intf, const Ipv4Addr *dstAddr, uint dstPort, uint srcPort, NetBuf *pkt)
{
    // UDP Header
    pkt->start -= sizeof(UdpHeader);

    UdpHeader *hdr = (UdpHeader *)pkt->start;
    hdr->srcPort = NetSwap16(srcPort);
    hdr->dstPort = NetSwap16(dstPort);
    hdr->len = NetSwap16(pkt->end - pkt->start);
    hdr->checksum = 0;  // don't compute checksum yet

    UdpPrint(pkt);

    Ipv4SendIntf(intf, dstAddr, dstAddr, IP_PROTOCOL_UDP, pkt);
}

// ------------------------------------------------------------------------------------------------
void UdpPrint(const NetBuf *pkt)
{
    if (~g_netTrace & TRACE_TRANSPORT)
    {
        return;
    }

    if (pkt->start + sizeof(UdpHeader) > pkt->end)
    {
        return;
    }

    const UdpHeader *hdr = (const UdpHeader *)pkt->start;

    u16 srcPort = NetSwap16(hdr->srcPort);
    u16 dstPort = NetSwap16(hdr->dstPort);
    u16 len = NetSwap16(hdr->len);
    u16 checksum = NetSwap16(hdr->checksum);

    ConsolePrint("  UDP: src=%d dst=%d len=%d checksum=%d\n",
        srcPort, dstPort, len, checksum);
}
