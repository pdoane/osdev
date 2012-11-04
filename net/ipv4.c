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
void Ipv4Recv(NetIntf *intf, NetBuf *pkt)
{
    Ipv4Print(pkt);

    // Validate packet header
    if (pkt->start + sizeof(Ipv4Header) > pkt->end)
    {
        return;
    }

    const Ipv4Header *hdr = (const Ipv4Header *)pkt->start;

    uint version = (hdr->verIhl >> 4) & 0xf;
    if (version != 4)
    {
        return;
    }

    // Fragments
    u16 fragment = NetSwap16(hdr->offset) & 0x1fff;

    // Fragments are not handled yet
    if (fragment)
    {
        return;
    }

    // Jump to packet data
    uint ihl = (hdr->verIhl) & 0xf;

    // Update packet end
    u8 *ipEnd = pkt->start + NetSwap16(hdr->len);
    if (ipEnd > pkt->end)
    {
        ConsolePrint("IP Packet too long\n");
        return;
    }

    pkt->start += ihl << 2;
    pkt->end = ipEnd;

    // Dispatch based on protocol
    switch (hdr->protocol)
    {
    case IP_PROTOCOL_ICMP:
        IcmpRecv(intf, hdr, pkt);
        break;

    case IP_PROTOCOL_TCP:
        TcpRecv(intf, hdr, pkt);
        break;

    case IP_PROTOCOL_UDP:
        UdpRecv(intf, hdr, pkt);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void Ipv4SendIntf(NetIntf *intf, const Ipv4Addr *nextAddr,
    const Ipv4Addr *dstAddr, u8 protocol, NetBuf *pkt)
{
    // IPv4 Header
    pkt->start -= sizeof(Ipv4Header);

    Ipv4Header *hdr = (Ipv4Header *)pkt->start;
    hdr->verIhl = (4 << 4) | 5;
    hdr->tos = 0;
    hdr->len = NetSwap16(pkt->end - pkt->start);
    hdr->id = NetSwap16(0);
    hdr->offset = NetSwap16(0);
    hdr->ttl = 64;
    hdr->protocol = protocol;
    hdr->checksum = 0;
    hdr->src = intf->ipAddr;
    hdr->dst = *dstAddr;

    uint checksum = NetChecksum(pkt->start, pkt->start + sizeof(Ipv4Header));
    hdr->checksum = NetSwap16(checksum);

    Ipv4Print(pkt);

    intf->send(intf, nextAddr, ET_IPV4, pkt);
}

// ------------------------------------------------------------------------------------------------
void Ipv4Send(const Ipv4Addr *dstAddr, u8 protocol, NetBuf *pkt)
{
    const NetRoute *route = NetFindRoute(dstAddr);

    if (route)
    {
        const Ipv4Addr *nextAddr = NetNextAddr(route, dstAddr);

        Ipv4SendIntf(route->intf, nextAddr, dstAddr, protocol, pkt);
    }
}

// ------------------------------------------------------------------------------------------------
void Ipv4Print(const NetBuf *pkt)
{
    if (~g_netTrace & TRACE_NET)
    {
        return;
    }

    if (pkt->start + sizeof(Ipv4Header) > pkt->end)
    {
        return;
    }

    const Ipv4Header *hdr = (const Ipv4Header *)pkt->start;

    uint version = (hdr->verIhl >> 4) & 0xf;
    uint ihl = (hdr->verIhl) & 0xf;
    uint dscp = (hdr->tos >> 2) & 0x3f;
    uint ecn = (hdr->tos) & 0x3;
    u16 len = NetSwap16(hdr->len);
    u16 id = NetSwap16(hdr->id);
    u16 fragment = NetSwap16(hdr->offset) & 0x1fff;
    u8 ttl = hdr->ttl;
    u8 protocol = hdr->protocol;
    u16 checksum = NetSwap16(hdr->checksum);

    uint checksum2 = NetChecksum(pkt->start, pkt->start + sizeof(Ipv4Header));

    char srcAddrStr[IPV4_ADDR_STRING_SIZE];
    char dstAddrStr[IPV4_ADDR_STRING_SIZE];
    Ipv4AddrToStr(srcAddrStr, sizeof(srcAddrStr), &hdr->src);
    Ipv4AddrToStr(dstAddrStr, sizeof(dstAddrStr), &hdr->dst);

    ConsolePrint(" IPv4: version=%d ihl=%d dscp=%d ecn=%d\n",
            version, ihl, dscp, ecn);
    ConsolePrint(" IPv4: len=%d, id=%d, fragment=%d, ttl=%d, protocol=%d, checksum=%d%c\n",
            len, id, fragment, ttl, protocol, checksum,
            checksum2 ? '!' : ' ');
    ConsolePrint(" IPv4: dst=%s src=%s\n",
            dstAddrStr, srcAddrStr);
}
