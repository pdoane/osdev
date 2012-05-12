// ------------------------------------------------------------------------------------------------
// ipv4.c
// ------------------------------------------------------------------------------------------------

#include "ipv4.h"
#include "console.h"
#include "icmp.h"
#include "net.h"

// ------------------------------------------------------------------------------------------------
#define IP_PROTOCOL_ICMP                1
#define IP_PROTOCOL_TCP                 6
#define IP_PROTOCOL_UDP                 17

// ------------------------------------------------------------------------------------------------
void ipv4_print(u8* pkt, uint len)
{
    if (!net_trace)
    {
        return;
    }

    if (len < 20)
    {
        return;
    }

    uint version = (pkt[0] >> 4) & 0xf;
    uint ihl = (pkt[0]) & 0xf;
    uint dscp = (pkt[1] >> 2) & 0x3f;
    uint ecn = (pkt[1]) & 0x3;
    u16 packet_len = (pkt[2] << 8) | pkt[3];
    u16 id = (pkt[4] << 8) | pkt[5];
    u16 fragment = (pkt[6] << 8) | pkt[7];
    u8 ttl = pkt[8];
    u8 protocol = pkt[9];
    u16 checksum = (pkt[10] << 8) | pkt[11];
    u8* src_addr = pkt + 12;
    u8* dst_addr = pkt + 16;

    console_print(" IPv4: version=%d ihl=%d dscp=%d ecn=%d\n",
            version, ihl, dscp, ecn);
    console_print(" IPv4: len=%d, id=%d, fragment=%d, ttl=%d, protocol=%d, checksum=%d\n",
            packet_len, id, fragment, ttl, protocol, checksum);
    console_print(" IPv4: source=%d.%d.%d.%d, dest=%d.%d.%d.%d\n",
            src_addr[0], src_addr[1], src_addr[2], src_addr[3],
            dst_addr[0], dst_addr[1], dst_addr[2], dst_addr[3]);
}

// ------------------------------------------------------------------------------------------------
void ipv4_rx(u8* pkt, uint len)
{
    ipv4_print(pkt, len);

    // Validate packet length
    if (len < 20)
    {
        return;
    }

    // Validate packet header
    uint version = (pkt[0] >> 4) & 0xf;
    if (version != 4)
    {
        return;
    }

    //uint ihl = (pkt[0]) & 0xf;
    //uint dscp = (pkt[1] >> 2) & 0x3f;
    //uint ecn = (pkt[1]) & 0x3;
    u16 packet_len = (pkt[2] << 8) | pkt[3];
    //u16 id = (pkt[4] << 8) | pkt[5];
    u16 fragment = (pkt[6] << 8) | pkt[7];
    //u8 ttl = pkt[8];
    u8 protocol = pkt[9];
    //u16 checksum = (pkt[10] << 8) | pkt[11];
    //u8* src_addr = pkt + 12;
    //u8* dst_addr = pkt + 16;

    // Fragments are not handled yet
    if (fragment)
    {
        return;
    }

    // Jump to packet data
    //u8* data = pkt + (ihl << 2);

    // Dispatch based on protocol
    switch (protocol)
    {
    case IP_PROTOCOL_ICMP:
        icmp_rx(pkt, packet_len);	// Send the base IPv4 packet
        break;

    case IP_PROTOCOL_TCP:
        break;

    case IP_PROTOCOL_UDP:
        break;
    }
}

// ------------------------------------------------------------------------------------------------
u16 ipv4_checksum(u8* data, uint len)
{
    uint sum = 0;
    u16* p = (u16*)data;

    while (len > 1)
    {
        sum += *p++;
        len -= 2;
    }

    if (len)
    {
        sum += *(u8*)p;
    }

    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    u16 temp = ~sum;
    return ((temp & 0x00ff) << 8) | ((temp & 0xff00) >> 8);
}
