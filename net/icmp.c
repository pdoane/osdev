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
void icmp_print(const u8* pkt, const u8* end)
{
    if (~net_trace & TRACE_NET)
    {
        return;
    }

    if (pkt + 8 > end)
    {
        return;
    }

    u8 type = pkt[0];
    u8 code = pkt[1];
    u16 checksum = (pkt[2] << 8) | pkt[3];
    u16 id = (pkt[4] << 8) | pkt[5];
    u16 sequence = (pkt[6] << 8) | pkt[7];

    uint checksum2 = net_checksum(pkt, end);

    console_print("  ICMP: type=%d code=%d id=%d sequence=%d len=%d checksum=%d%c\n",
            type, code, id, sequence, end - pkt, checksum,
            checksum2 ? '!' : ' ');
}

// ------------------------------------------------------------------------------------------------
static void icmp_echo_reply(const IPv4_Addr* dst_addr, u16 id, u16 sequence,
    const u8* data, const u8* data_end)
{
    uint data_len = data_end - data;

    NetBuf* buf = net_alloc_packet();
    u8* pkt = (u8*)(buf + 1);

    pkt[0] = ICMP_TYPE_ECHO_REPLY;
    pkt[1] = 0;
    pkt[2] = 0;
    pkt[3] = 0;
    pkt[4] = (id >> 8) & 0xff;
    pkt[5] = (id) & 0xff;
    pkt[6] = (sequence >> 8) & 0xff;
    pkt[7] = (sequence) & 0xff;
    memcpy(pkt + 8, data, data_len);
    u8* end = pkt + 8 + data_len;

    uint checksum = net_checksum(pkt, end);
    pkt[2] = (checksum >> 8) & 0xff;
    pkt[3] = (checksum) & 0xff;

    icmp_print(pkt, end);
    ipv4_tx(dst_addr, IP_PROTOCOL_ICMP, pkt, end);
}

// ------------------------------------------------------------------------------------------------
void icmp_echo_request(const IPv4_Addr* dst_addr, u16 id, u16 sequence,
    const u8* data, const u8* data_end)
{
    uint data_len = data_end - data;

    NetBuf* buf = net_alloc_packet();
    u8* pkt = (u8*)(buf + 1);

    pkt[0] = ICMP_TYPE_ECHO_REQUEST;
    pkt[1] = 0;
    pkt[2] = 0;
    pkt[3] = 0;
    pkt[4] = (id >> 8) & 0xff;
    pkt[5] = (id) & 0xff;
    pkt[6] = (sequence >> 8) & 0xff;
    pkt[7] = (sequence) & 0xff;
    memcpy(pkt + 8, data, data_len);
    u8* end = pkt + 8 + data_len;

    uint checksum = net_checksum(pkt, end);
    pkt[2] = (checksum >> 8) & 0xff;
    pkt[3] = (checksum) & 0xff;

    icmp_print(pkt, end);
    ipv4_tx(dst_addr, IP_PROTOCOL_ICMP, pkt, end);
}

// ------------------------------------------------------------------------------------------------
void icmp_rx(Net_Intf* intf, u8* pkt, u8* end)
{
    if (pkt + 20 > end)
    {
        return;
    }

    uint ihl = (pkt[0]) & 0xf;
    const IPv4_Addr* src_addr = (const IPv4_Addr*)(pkt + 12);
    //const IPv4_Addr* dst_ip = (const IPv4_Addr*)(pkt + 16);

    // Jump to sub-packet data
    pkt += ihl << 2;
    icmp_print(pkt, end);

    if (pkt + 8 > end)
    {
        return;
    }

    // Decode ICMP data
    u8 type = pkt[0];
    //u8 code = pkt[1];
    //u16 checksum = (pkt[2] << 8) | pkt[3];
    u16 id = (pkt[4] << 8) | pkt[5];
    u16 sequence = (pkt[6] << 8) | pkt[7];

    if (type == ICMP_TYPE_ECHO_REQUEST)
    {
        char src_addr_str[IPV4_ADDR_STRING_SIZE];
        ipv4_addr_to_str(src_addr_str, sizeof(src_addr_str), src_addr);

        console_print("Echo request from %s\n", src_addr_str);
        icmp_echo_reply(src_addr, id, sequence, pkt + 8, end);
    }
    else if (type == ICMP_TYPE_ECHO_REPLY)
    {
        char src_addr_str[IPV4_ADDR_STRING_SIZE];
        ipv4_addr_to_str(src_addr_str, sizeof(src_addr_str), src_addr);

        console_print("Echo reply from %s\n", src_addr_str);
    }
}
