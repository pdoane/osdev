// ------------------------------------------------------------------------------------------------
// net/icmp.c
// ------------------------------------------------------------------------------------------------

#include "net/icmp.h"
#include "net/buf.h"
#include "net/checksum.h"
#include "net/ipv4.h"
#include "net/net.h"
#include "console/console.h"
#include "stdlib/string.h"

// ------------------------------------------------------------------------------------------------
#define ICMP_TYPE_ECHO_REPLY            0
#define ICMP_TYPE_DEST_UNREACHABLE      3
#define ICMP_TYPE_SOUCE_QUENCH          4
#define ICMP_TYPE_REDIRECT_MSG          5
#define ICMP_TYPE_ECHO_REQUEST          8
#define ICMP_TYPE_ROUTER_ADVERTISEMENT  9
#define ICMP_TYPE_ROUTER_SOLICITATION   10
#define ICMP_TYPE_TIME_EXCEEDED         11
#define ICMP_TYPE_BAD_PARAM             12
#define ICMP_TYPE_TIMESTAMP             13
#define ICMP_TYPE_TIMESTAMP_REPLY       14
#define ICMP_TYPE_INFO_REQUEST          15
#define ICMP_TYPE_INFO_REPLY            16
#define ICMP_TYPE_ADDR_MASK_REQUEST     17
#define ICMP_TYPE_ADDR_MASK_REPLY       18
#define ICMP_TYPE_TRACEROUTE            30

// ------------------------------------------------------------------------------------------------
void IcmpPrint(const NetBuf *pkt)
{
    if (~g_netTrace & TRACE_NET)
    {
        return;
    }

    if (pkt->start + 8 > pkt->end)
    {
        return;
    }

    u8 *data = pkt->start;
    u8 type = data[0];
    u8 code = data[1];
    u16 checksum = (data[2] << 8) | data[3];
    u16 id = (data[4] << 8) | data[5];
    u16 sequence = (data[6] << 8) | data[7];

    uint checksum2 = NetChecksum(pkt->start, pkt->end);

    ConsolePrint("  ICMP: type=%d code=%d id=%d sequence=%d len=%d checksum=%d%c\n",
            type, code, id, sequence, pkt->end - pkt->start, checksum,
            checksum2 ? '!' : ' ');
}

// ------------------------------------------------------------------------------------------------
static void IcmpEchoReply(const Ipv4Addr *dstAddr, u16 id, u16 sequence,
    const u8 *echoData, const u8 *echoEnd)
{
    uint echoLen = echoEnd - echoData;

    NetBuf *pkt = NetAllocBuf();

    u8 *data = pkt->start;
    data[0] = ICMP_TYPE_ECHO_REPLY;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    data[4] = (id >> 8) & 0xff;
    data[5] = (id) & 0xff;
    data[6] = (sequence >> 8) & 0xff;
    data[7] = (sequence) & 0xff;
    memcpy(data + 8, data, echoLen);
    pkt->end += 8 + echoLen;

    uint checksum = NetChecksum(pkt->start, pkt->end);
    data[2] = (checksum >> 8) & 0xff;
    data[3] = (checksum) & 0xff;

    IcmpPrint(pkt);
    Ipv4Send(dstAddr, IP_PROTOCOL_ICMP, pkt);
}

// ------------------------------------------------------------------------------------------------
void IcmpEchoRequest(const Ipv4Addr *dstAddr, u16 id, u16 sequence,
    const u8 *echoData, const u8 *echoEnd)
{
    uint echoLen = echoEnd - echoData;

    NetBuf *pkt = NetAllocBuf();

    u8 *data = pkt->start;
    data[0] = ICMP_TYPE_ECHO_REQUEST;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    data[4] = (id >> 8) & 0xff;
    data[5] = (id) & 0xff;
    data[6] = (sequence >> 8) & 0xff;
    data[7] = (sequence) & 0xff;
    memcpy(data + 8, echoData, echoLen);
    pkt->end += 8 + echoLen;

    uint checksum = NetChecksum(pkt->start, pkt->end);
    data[2] = (checksum >> 8) & 0xff;
    data[3] = (checksum) & 0xff;

    IcmpPrint(pkt);
    Ipv4Send(dstAddr, IP_PROTOCOL_ICMP, pkt);
}

// ------------------------------------------------------------------------------------------------
void IcmpRecv(NetIntf *intf, const Ipv4Header *ipHdr, NetBuf *pkt)
{
    IcmpPrint(pkt);

    if (pkt->start + 8 > pkt->end)
    {
        return;
    }

    // Decode ICMP data
    const u8 *data = pkt->start;
    u8 type = data[0];
    //u8 code = data[1];
    //u16 checksum = (data[2] << 8) | data[3];
    u16 id = (data[4] << 8) | data[5];
    u16 sequence = (data[6] << 8) | data[7];

    if (type == ICMP_TYPE_ECHO_REQUEST)
    {
        char srcAddrStr[IPV4_ADDR_STRING_SIZE];
        Ipv4AddrToStr(srcAddrStr, sizeof(srcAddrStr), &ipHdr->src);

        ConsolePrint("Echo request from %s\n", srcAddrStr);
        IcmpEchoReply(&ipHdr->src, id, sequence, data + 8, pkt->end);
    }
    else if (type == ICMP_TYPE_ECHO_REPLY)
    {
        char srcAddrStr[IPV4_ADDR_STRING_SIZE];
        Ipv4AddrToStr(srcAddrStr, sizeof(srcAddrStr), &ipHdr->src);

        ConsolePrint("Echo reply from %s\n", srcAddrStr);
    }
}
