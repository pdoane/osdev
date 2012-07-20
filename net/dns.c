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

typedef struct DNS_Header
{
    u16 id;
    u16 flags;
    u16 question_count;
    u16 answer_count;
    u16 authority_count;
    u16 additional_count;
} PACKED DNS_Header;

// ------------------------------------------------------------------------------------------------
// Globals

IPv4_Addr dns_server;

// ------------------------------------------------------------------------------------------------
void dns_rx(Net_Intf* intf, const Net_Buf* pkt)
{
    dns_print(pkt);
}

// ------------------------------------------------------------------------------------------------
void dns_query_host(const char* host, uint id)
{
    // Skip request if not configured
    if (ipv4_addr_eq(&dns_server, &null_ipv4_addr))
    {
        return;
    }

    Net_Buf* pkt = net_alloc_buf();

    DNS_Header* hdr = (DNS_Header*)pkt->start;
    hdr->id = net_swap16(id);
    hdr->flags = net_swap16(0x0100);    // Recursion Desired
    hdr->question_count = net_swap16(1);
    hdr->answer_count = net_swap16(0);
    hdr->authority_count = net_swap16(0);
    hdr->additional_count = net_swap16(0);

    u8* q = pkt->start + sizeof(DNS_Header);
    uint host_len = strlen(host);
    if (host_len >= 256)
    {
        return;
    }

    // Convert hostname to DNS format
    u8* label_head = q++;
    const char* p = host;
    for (;;)
    {
        char c = *p++;

        if (c == '.' || c == '\0')
        {
            uint label_len = q - label_head - 1;
            *label_head = label_len;
            label_head = q;
        }

        *q++ = c;

        if (!c)
        {
            break;
        }
    }

    *(u16*)q = net_swap16(1);   // query type
    q += sizeof(u16);
    *(u16*)q = net_swap16(1);   // query class
    q += sizeof(u16);

    pkt->end = q;
    uint src_port = net_ephemeral_port();

    dns_print(pkt);
    udp_tx(&dns_server, PORT_DNS, src_port, pkt);
}

// ------------------------------------------------------------------------------------------------
static const u8* dns_print_host(const Net_Buf* pkt, const u8* p, bool first)
{
    for (;;)
    {
        u8 count = *p++;

        if (count >= 64)
        {
            u8 n = *p++;
            uint offset = ((count & 0x3f) << 6) | n;

            dns_print_host(pkt, pkt->start + offset, first);
            return p;
        }
        else if (count > 0)
        {
            if (!first)
            {
                console_print(".");
            }

            char buf[64];
            memcpy(buf, p, count);
            buf[count] = '\0';
            console_print(buf);

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
static const u8* dns_print_query(const Net_Buf* pkt, const u8* p)
{
    console_print("    Query: ");
    p = dns_print_host(pkt, p, true);

    u16 query_type = (p[0] << 8) | p[1];
    u16 query_class = (p[2] << 8) | p[3];
    p += 4;

    console_print(" type=%d class=%d\n", query_type, query_class);

    return p;
}

// ------------------------------------------------------------------------------------------------
static const u8* dns_print_rr(const char* hdr, const Net_Buf* pkt, const u8* p)
{
    console_print("    %s: ", hdr);
    p = dns_print_host(pkt, p, true);

    u16 query_type = (p[0] << 8) | p[1];
    u16 query_class = (p[2] << 8) | p[3];
    u32 ttl = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
    u16 data_len = (p[8] << 8) | p[9];
    p += 10;

    const u8* data = p;

    console_print(" type=%d class=%d ttl=%d data_len=%d ", query_type, query_class, ttl, data_len);

    if (query_type == 1 && data_len == 4)
    {
        const IPv4_Addr* addr = (const IPv4_Addr*)data;
        char addr_str[IPV4_ADDR_STRING_SIZE];

        ipv4_addr_to_str(addr_str, sizeof(addr_str), addr);
        console_print("%s", addr_str);
    }
    else if (query_type == 2)
    {
        dns_print_host(pkt, data, true);
    }

    console_print("\n");

    return p + data_len;
}

// ------------------------------------------------------------------------------------------------
void dns_print(const Net_Buf* pkt)
{
    const DNS_Header* hdr = (const DNS_Header*)pkt->start;

    u16 id = net_swap16(hdr->id);
    u16 flags = net_swap16(hdr->flags);
    u16 question_count = net_swap16(hdr->question_count);
    u16 answer_count = net_swap16(hdr->answer_count);
    u16 authority_count = net_swap16(hdr->authority_count);
    u16 additional_count = net_swap16(hdr->additional_count);

    console_print("   DNS: id=%d flags=%04x questions=%d answers=%d authorities=%d additional=%d\n",
        id, flags, question_count, answer_count, authority_count, additional_count);

    const u8* p = pkt->start + sizeof(DNS_Header);

    for (uint i = 0; i < question_count; ++i)
    {
        p = dns_print_query(pkt, p);
    }

    for (uint i = 0; i < answer_count; ++i)
    {
        p = dns_print_rr("Ans", pkt, p);
    }

    for (uint i = 0; i < authority_count; ++i)
    {
        p = dns_print_rr("Auth", pkt, p);
    }

    for (uint i = 0; i < additional_count; ++i)
    {
        p = dns_print_rr("Add", pkt, p);
    }
}
