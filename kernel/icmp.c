// ------------------------------------------------------------------------------------------------
// icmp.c
// ------------------------------------------------------------------------------------------------

#include "icmp.h"
#include "arp.h"
#include "console.h"
#include "eth.h"
#include "net.h"
#include "ipv4.h"
#include "string.h"

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
void icmp_print(const u8* pkt, uint len)
{
    if (!net_trace)
    {
        return;
    }

    if (len < 8)
    {
        return;
    }

    u8 type = pkt[0];
    u8 code = pkt[1];
    u16 checksum = (pkt[2] << 8) | pkt[3];
    u16 id = (pkt[4] << 8) | pkt[5];
    u16 sequence = (pkt[6] << 8) | pkt[7];

    console_print("  ICMP: type=%d code=%d checksum=%d id=%d sequence=%d\n",
            type, code, checksum, id, sequence);
}

// ------------------------------------------------------------------------------------------------
static void icmp_echo_reply(const IPv4_Addr* dst_ip, u16 id, u16 sequence, const u8* echo_data, uint echo_data_len)
{
    u8 data[1500];

    const Eth_Addr* dst_mac = arp_lookup_mac(dst_ip);
    if (!dst_mac)
    {
        console_print(" Unknown IP, sending ARP request\n");
        arp_request(dst_ip);
        return;
    }

    u8* pkt = eth_encode_hdr(data, dst_mac, &net_local_mac, ET_IPV4);

    uint icmp_packet_size = 8 + echo_data_len;
    uint ip_packet_size = 20 + icmp_packet_size;

    // IPv4 Header
    pkt[0] = (4 << 4) | 5;
    pkt[1] = 0;
    pkt[2] = (ip_packet_size >> 8) & 0xff;
    pkt[3] = (ip_packet_size) & 0xff;
    pkt[4] = 0;
    pkt[5] = 0;
    pkt[6] = 0;
    pkt[7] = 0;
    pkt[8] = 64;
    pkt[9] = 1;
    pkt[10] = 0;
    pkt[11] = 0;
    *(IPv4_Addr*)(pkt + 12) = net_local_ip;
    *(IPv4_Addr*)(pkt + 16) = *dst_ip;

    uint checksum = ipv4_checksum(pkt, 20);
    pkt[10] = (checksum >> 8) & 0xff;
    pkt[11] = (checksum) & 0xff;

    // ICMP
    pkt[20] = ICMP_TYPE_ECHO_REPLY;
    pkt[21] = 0;
    pkt[22] = 0;
    pkt[23] = 0;
    pkt[24] = (id >> 8) & 0xff;
    pkt[25] = (id) & 0xff;
    pkt[26] = (sequence >> 8) & 0xff;
    pkt[27] = (sequence) & 0xff;
    memcpy(pkt + 28, echo_data, echo_data_len);

    checksum = ipv4_checksum(pkt + 20, icmp_packet_size);
    pkt[22] = (checksum >> 8) & 0xff;
    pkt[23] = (checksum) & 0xff;

    icmp_print(pkt + 20, icmp_packet_size);
    ipv4_print(pkt, ip_packet_size);

    uint len = 14 + ip_packet_size;
    net_tx(data, len);
}

// ------------------------------------------------------------------------------------------------
void icmp_rx(const u8* pkt, uint len)
{
    if (len < 20)
    {
        return;
    }

    uint ihl = (pkt[0]) & 0xf;
    const IPv4_Addr* src_ip = (const IPv4_Addr*)(pkt + 12);
    //const IPv4_Addr* dst_ip = (const IPv4_Addr*)(pkt + 16);

    // Jump to sub-packet data
    pkt += ihl << 2;
    len -= ihl << 2;
    icmp_print(pkt, len);

    if (len < 8)
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
        icmp_echo_reply(src_ip, id, sequence, pkt + 8, len - 8);
    }
}
