// ------------------------------------------------------------------------------------------------
// net/arp.c
// ------------------------------------------------------------------------------------------------

#include "net/arp.h"
#include "net/buf.h"
#include "net/eth.h"
#include "net/ipv4.h"
#include "net/net.h"
#include "net/swap.h"
#include "console/console.h"
#include "stdlib/string.h"

// ------------------------------------------------------------------------------------------------
// ARP Protocol
#define ARP_HTYPE_ETH       0x01

#define ARP_OP_REQUEST      0x01
#define ARP_OP_REPLY        0x02

// ------------------------------------------------------------------------------------------------
// ARP Cache
#define ARP_CACHE_SIZE      16

typedef struct ArpEntry
{
    EthAddr ha;
    Ipv4Addr pa;

    // deferred packet to send
    NetIntf *intf;
    u16 etherType;
    NetBuf *pkt;
} ArpEntry;

static ArpEntry s_arpCache[ARP_CACHE_SIZE];

// ------------------------------------------------------------------------------------------------
static void ArpPrint(const NetBuf *pkt)
{
    if (~g_netTrace & TRACE_LINK)
    {
        return;
    }

    if (pkt->start + sizeof(ArpHeader) > pkt->end)
    {
        return;
    }

    const u8 *data = pkt->start;
    const ArpHeader *hdr = (const ArpHeader *)data;

    u16 htype = NetSwap16(hdr->htype);
    u16 ptype = NetSwap16(hdr->ptype);
    u8 hlen = hdr->hlen;
    u8 plen = hdr->plen;
    u16 op = NetSwap16(hdr->op);
    ConsolePrint(" ARP: htype=0x%x ptype=0x%x hlen=%d plen=%d op=%d\n",
            htype, ptype, hlen, plen, op);

    if (htype == ARP_HTYPE_ETH && ptype == ET_IPV4 && pkt->start + 28 <= pkt->end)
    {
        const EthAddr *sha = (const EthAddr *)(data + 8);
        const Ipv4Addr *spa = (const Ipv4Addr *)(data + 14);
        const EthAddr *tha = (const EthAddr *)(data + 18);
        const Ipv4Addr *tpa = (const Ipv4Addr *)(data + 24);

        char shaStr[ETH_ADDR_STRING_SIZE];
        char spaStr[IPV4_ADDR_STRING_SIZE];
        char thaStr[ETH_ADDR_STRING_SIZE];
        char tpaStr[IPV4_ADDR_STRING_SIZE];

        EthAddrToStr(shaStr, sizeof(shaStr), sha);
        Ipv4AddrToStr(spaStr, sizeof(spaStr), spa);
        EthAddrToStr(thaStr, sizeof(thaStr), tha);
        Ipv4AddrToStr(tpaStr, sizeof(tpaStr), tpa);

        ConsolePrint(" ARP: %s spa=%s\n", shaStr, spaStr);
        ConsolePrint(" ARP: %s tpa=%s\n", thaStr, tpaStr);
    }
}

// ------------------------------------------------------------------------------------------------
static void ArpSend(NetIntf *intf, uint op, const EthAddr *tha, const Ipv4Addr *tpa)
{
    NetBuf *pkt = NetAllocBuf();
    u8 *data = pkt->start;

    // HTYPE
    data[0] = (ARP_HTYPE_ETH >> 8) & 0xff;
    data[1] = (ARP_HTYPE_ETH) & 0xff;

    // PTYPE
    data[2] = (ET_IPV4 >> 8) & 0xff;
    data[3] = (ET_IPV4) & 0xff;

    // HLEN
    data[4] = sizeof(EthAddr);

    // PLEN
    data[5] = sizeof(Ipv4Addr);

    // Operation
    data[6] = (op >> 8) & 0xff;
    data[7] = (op) & 0xff;

    // SHA
    *(EthAddr *)(data + 8) = intf->ethAddr;

    // SPA
    *(Ipv4Addr *)(data + 14) = intf->ipAddr;

    // THA
    if (op == ARP_OP_REQUEST)
    {
        *(EthAddr *)(data + 18) = g_nullEthAddr;
    }
    else
    {
        *(EthAddr *)(data + 18) = *tha;
    }

    // TPA
    *(Ipv4Addr *)(data + 24) = *tpa;

    pkt->end += 28;

    // Transmit packet
    ArpPrint(pkt);
    intf->send(intf, tha, ET_ARP, pkt);
}

// ------------------------------------------------------------------------------------------------
static ArpEntry *ArpLookup(const Ipv4Addr *pa)
{
    ArpEntry *entry = s_arpCache;
    ArpEntry *end = entry + ARP_CACHE_SIZE;
    for (; entry != end; ++entry)
    {
        if (Ipv4AddrEq(&entry->pa, pa))
        {
            return entry;
        }
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
static ArpEntry *ArpAdd(const EthAddr *ha, const Ipv4Addr *pa)
{
    // TODO - handle overflow
    ArpEntry *entry = s_arpCache;
    ArpEntry *end = entry + ARP_CACHE_SIZE;
    for (; entry != end; ++entry)
    {
        if (!entry->pa.u.bits)
        {
            break;
        }
    }

    if (entry != end)
    {
        entry->ha = *ha;
        entry->pa = *pa;
        return entry;
    }

    ConsolePrint("Ran out of ARP entries\n");
    return 0;
}

// ------------------------------------------------------------------------------------------------
void ArpRequest(NetIntf *intf, const Ipv4Addr *tpa, u16 etherType, NetBuf *pkt)
{
    ArpEntry *entry = ArpLookup(tpa);
    if (!entry)
    {
        entry = ArpAdd(&g_nullEthAddr, tpa);
    }

    if (entry)
    {
        // Drop any packet already queued
        if (entry->pkt)
        {
            NetReleaseBuf(entry->pkt);
        }

        entry->intf = intf;
        entry->etherType = etherType;
        entry->pkt = pkt;
        ArpSend(intf, ARP_OP_REQUEST, &g_broadcastEthAddr, tpa);
    }
}

// ------------------------------------------------------------------------------------------------
void ArpReply(NetIntf *intf, const EthAddr *tha, const Ipv4Addr *tpa)
{
    ArpSend(intf, ARP_OP_REPLY, tha, tpa);
}

// ------------------------------------------------------------------------------------------------
void ArpInit()
{
    // Clear cache of all entries
    ArpEntry *entry = s_arpCache;
    ArpEntry *end = entry + ARP_CACHE_SIZE;
    for (; entry != end; ++entry)
    {
        memset(&entry->ha, 0, sizeof(EthAddr));
        memset(&entry->pa, 0, sizeof(Ipv4Addr));
    }
}

// ------------------------------------------------------------------------------------------------
const EthAddr *ArpLookupEthAddr(const Ipv4Addr *pa)
{
    ArpEntry *entry = ArpLookup(pa);
    if (entry)
    {
        return &entry->ha;
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
void ArpRecv(NetIntf *intf, NetBuf *pkt)
{
    ArpPrint(pkt);

    // Decode Header
    if (pkt->start + sizeof(ArpHeader) > pkt->end)
    {
        return;
    }

    const u8 *data = pkt->start;
    const ArpHeader *hdr = (const ArpHeader *)data;

    u16 htype = NetSwap16(hdr->htype);
    u16 ptype = NetSwap16(hdr->ptype);
    u16 op = NetSwap16(hdr->op);

    // Skip packets that are not Ethernet, IPv4, or well-formed
    if (htype != ARP_HTYPE_ETH || ptype != ET_IPV4 || pkt->start + 28 > pkt->end)
    {
        return;
    }

    // Decode addresses
    const EthAddr *sha = (const EthAddr *)(data + 8);
    const Ipv4Addr *spa = (const Ipv4Addr *)(data + 14);
    //const EthAddr *tha = (const EthAddr *)(data + 18);
    const Ipv4Addr *tpa = (const Ipv4Addr *)(data + 24);

    // Update existing entry if we know about this source IP address
    bool merge = false;
    ArpEntry *entry = ArpLookup(spa);
    if (entry)
    {
        entry->ha = *sha;
        merge = true;

        // Send deferred packet
        if (entry->pkt)
        {
            EthSendIntf(entry->intf, spa, entry->etherType, entry->pkt);

            entry->intf = 0;
            entry->etherType = 0;
            entry->pkt = 0;
        }
    }

    // Check if this ARP packet is targeting our IP
    if (Ipv4AddrEq(tpa, &intf->ipAddr))
    {
        // Add a new entry if we didn't update earlier.
        if (!merge)
        {
            ArpAdd(sha, spa);
        }

        // Respond to requests.
        if (op == ARP_OP_REQUEST)
        {
            ArpReply(intf, sha, spa);
        }
    }
}
