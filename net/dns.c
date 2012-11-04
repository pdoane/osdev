// ------------------------------------------------------------------------------------------------
// net/dns.c
// ------------------------------------------------------------------------------------------------

#include "net/dns.h"
#include "net/buf.h"
#include "net/port.h"
#include "net/swap.h"
#include "net/udp.h"
#include "console/console.h"
#include "stdlib/string.h"

// ------------------------------------------------------------------------------------------------
// DNS Header

typedef struct DnsHeader
{
    u16 id;
    u16 flags;
    u16 questionCount;
    u16 answerCount;
    u16 authorityCount;
    u16 additionalCount;
} PACKED DnsHeader;

// ------------------------------------------------------------------------------------------------
// Globals

Ipv4Addr g_dnsServer;

// ------------------------------------------------------------------------------------------------
void DnsRecv(NetIntf *intf, const NetBuf *pkt)
{
    DnsPrint(pkt);
}

// ------------------------------------------------------------------------------------------------
void DnsQueryHost(const char *host, uint id)
{
    // Skip request if not configured
    if (Ipv4AddrEq(&g_dnsServer, &g_nullIpv4Addr))
    {
        return;
    }

    NetBuf *pkt = NetAllocBuf();

    DnsHeader *hdr = (DnsHeader *)pkt->start;
    hdr->id = NetSwap16(id);
    hdr->flags = NetSwap16(0x0100);    // Recursion Desired
    hdr->questionCount = NetSwap16(1);
    hdr->answerCount = NetSwap16(0);
    hdr->authorityCount = NetSwap16(0);
    hdr->additionalCount = NetSwap16(0);

    u8 *q = pkt->start + sizeof(DnsHeader);
    uint hostLen = strlen(host);
    if (hostLen >= 256)
    {
        return;
    }

    // Convert hostname to DNS format
    u8 *labelHead = q++;
    const char *p = host;
    for (;;)
    {
        char c = *p++;

        if (c == '.' || c == '\0')
        {
            uint labelLen = q - labelHead - 1;
            *labelHead = labelLen;
            labelHead = q;
        }

        *q++ = c;

        if (!c)
        {
            break;
        }
    }

    *(u16 *)q = NetSwap16(1);   // query type
    q += sizeof(u16);
    *(u16 *)q = NetSwap16(1);   // query class
    q += sizeof(u16);

    pkt->end = q;
    uint srcPort = NetEphemeralPort();

    DnsPrint(pkt);
    UdpSend(&g_dnsServer, PORT_DNS, srcPort, pkt);
}

// ------------------------------------------------------------------------------------------------
static const u8 *DnsPrintHost(const NetBuf *pkt, const u8 *p, bool first)
{
    for (;;)
    {
        u8 count = *p++;

        if (count >= 64)
        {
            u8 n = *p++;
            uint offset = ((count & 0x3f) << 6) | n;

            DnsPrintHost(pkt, pkt->start + offset, first);
            return p;
        }
        else if (count > 0)
        {
            if (!first)
            {
                ConsolePrint(".");
            }

            char buf[64];
            memcpy(buf, p, count);
            buf[count] = '\0';
            ConsolePrint(buf);

            p += count;
            first = false;
        }
        else
        {
            return p;
        }
    }
}

// ------------------------------------------------------------------------------------------------
static const u8 *DnsPrintQuery(const NetBuf *pkt, const u8 *p)
{
    ConsolePrint("    Query: ");
    p = DnsPrintHost(pkt, p, true);

    u16 queryType = (p[0] << 8) | p[1];
    u16 queryClass = (p[2] << 8) | p[3];
    p += 4;

    ConsolePrint(" type=%d class=%d\n", queryType, queryClass);

    return p;
}

// ------------------------------------------------------------------------------------------------
static const u8 *DnsPrintRR(const char *hdr, const NetBuf *pkt, const u8 *p)
{
    ConsolePrint("    %s: ", hdr);
    p = DnsPrintHost(pkt, p, true);

    u16 queryType = (p[0] << 8) | p[1];
    u16 queryClass = (p[2] << 8) | p[3];
    u32 ttl = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
    u16 dataLen = (p[8] << 8) | p[9];
    p += 10;

    const u8 *data = p;

    ConsolePrint(" type=%d class=%d ttl=%d dataLen=%d ", queryType, queryClass, ttl, dataLen);

    if (queryType == 1 && dataLen == 4)
    {
        const Ipv4Addr *addr = (const Ipv4Addr *)data;
        char addrStr[IPV4_ADDR_STRING_SIZE];

        Ipv4AddrToStr(addrStr, sizeof(addrStr), addr);
        ConsolePrint("%s", addrStr);
    }
    else if (queryType == 2)
    {
        DnsPrintHost(pkt, data, true);
    }

    ConsolePrint("\n");

    return p + dataLen;
}

// ------------------------------------------------------------------------------------------------
void DnsPrint(const NetBuf *pkt)
{
    const DnsHeader *hdr = (const DnsHeader *)pkt->start;

    u16 id = NetSwap16(hdr->id);
    u16 flags = NetSwap16(hdr->flags);
    u16 questionCount = NetSwap16(hdr->questionCount);
    u16 answerCount = NetSwap16(hdr->answerCount);
    u16 authorityCount = NetSwap16(hdr->authorityCount);
    u16 additionalCount = NetSwap16(hdr->additionalCount);

    ConsolePrint("   DNS: id=%d flags=%04x questions=%d answers=%d authorities=%d additional=%d\n",
        id, flags, questionCount, answerCount, authorityCount, additionalCount);

    const u8 *p = pkt->start + sizeof(DnsHeader);

    for (uint i = 0; i < questionCount; ++i)
    {
        p = DnsPrintQuery(pkt, p);
    }

    for (uint i = 0; i < answerCount; ++i)
    {
        p = DnsPrintRR("Ans", pkt, p);
    }

    for (uint i = 0; i < authorityCount; ++i)
    {
        p = DnsPrintRR("Auth", pkt, p);
    }

    for (uint i = 0; i < additionalCount; ++i)
    {
        p = DnsPrintRR("Add", pkt, p);
    }
}
