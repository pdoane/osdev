// ------------------------------------------------------------------------------------------------
// net/eth.h
// ------------------------------------------------------------------------------------------------

#include "net/eth.h"
#include "net/arp.h"
#include "net/ipv4.h"
#include "net/ipv6.h"
#include "net/net.h"
#include "net/swap.h"
#include "console/console.h"

// ------------------------------------------------------------------------------------------------
static bool EthDecode(EthPacket *ep, NetBuf *pkt)
{
    // Decode header
    if (pkt->start + sizeof(EthHeader) > pkt->end)
    {
        return false;
    }

    u8 *data = pkt->start;
    EthHeader *hdr = (EthHeader *)data;
    ep->hdr = hdr;

    // Determine which frame type is being used.
    u16 n = NetSwap16(hdr->etherType);
    if (n <= 1500 && pkt->start + 22 <= pkt->end)
    {
        // 802.2/802.3 encapsulation (RFC 1042)
        u8 dsap = data[14];
        u8 ssap = data[15];

        // Validate service access point
        if (dsap != 0xaa || ssap != 0xaa)
        {
            return false;
        }

        ep->etherType = (data[20] << 8) | data[21];
        ep->hdrLen = 22;
    }
    else
    {
        // Ethernet encapsulation (RFC 894)
        ep->etherType = n;
        ep->hdrLen = sizeof(EthHeader);
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
void EthRecv(NetIntf *intf, NetBuf *pkt)
{
    EthPrint(pkt);

    EthPacket ep;
    if (!EthDecode(&ep, pkt))
    {
        // Bad packet or one we don't care about (e.g. STP packets)
        return;
    }

    pkt->start += ep.hdrLen;

    // Dispatch packet based on protocol
    switch (ep.etherType)
    {
    case ET_ARP:
        ArpRecv(intf, pkt);
        break;

    case ET_IPV4:
        Ipv4Recv(intf, pkt);
        break;

    case ET_IPV6:
        Ipv6Recv(intf, pkt);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void EthSendIntf(NetIntf *intf, const void *dstAddr, u16 etherType, NetBuf *pkt)
{
    // Determine ethernet address by protocol of packet
    const EthAddr *dstEthAddr = 0;

    switch (etherType)
    {
    case ET_ARP:
        dstEthAddr = (const EthAddr *)dstAddr;
        break;

    case ET_IPV4:
        {
            const Ipv4Addr *dstIpv4Addr = (const Ipv4Addr *)dstAddr;

            if (Ipv4AddrEq(dstIpv4Addr, &g_broadcastIpv4Addr) ||
                Ipv4AddrEq(dstIpv4Addr, &intf->broadcastAddr))
            {
                // IP Broadcast -> Ethernet Broacast
                dstEthAddr = &g_broadcastEthAddr;
            }
            else
            {
                // Lookup Ethernet address in ARP cache
                dstEthAddr = ArpLookupEthAddr(dstIpv4Addr);
                if (!dstEthAddr)
                {
                    ArpRequest(intf, dstIpv4Addr, etherType, pkt);
                    return;
                }
            }
        }
        break;

    case ET_IPV6:
        break;
    }

    // Skip packets without a destination
    if (!dstEthAddr)
    {
        ConsolePrint("Dropped packet\n");
        return;
    }

    // Fill in ethernet header
    pkt->start -= sizeof(EthHeader);

    EthHeader *hdr = (EthHeader *)pkt->start;
    hdr->dst = *dstEthAddr;
    hdr->src = intf->ethAddr;
    hdr->etherType = NetSwap16(etherType);

    // Transmit
    EthPrint(pkt);
    intf->devSend(pkt);
}

// ------------------------------------------------------------------------------------------------
void EthPrint(NetBuf *pkt)
{
    if (~g_netTrace & TRACE_LINK)
    {
        return;
    }

    EthPacket ep;
    if (EthDecode(&ep, pkt))
    {
        char dstStr[ETH_ADDR_STRING_SIZE];
        char srcStr[ETH_ADDR_STRING_SIZE];

        EthAddrToStr(dstStr, sizeof(dstStr), &ep.hdr->dst);
        EthAddrToStr(srcStr, sizeof(srcStr), &ep.hdr->src);

        uint len = pkt->end - pkt->start - ep.hdrLen;
        ConsolePrint("ETH: dst=%s src=%s et=%04x len=%d\n", dstStr, srcStr, ep.etherType, len);
    }
}
