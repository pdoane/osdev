// ------------------------------------------------------------------------------------------------
// net/dhcp.c
// ------------------------------------------------------------------------------------------------

#include "net/dhcp.h"
#include "net/buf.h"
#include "net/dns.h"
#include "net/ipv4.h"
#include "net/net.h"
#include "net/port.h"
#include "net/route.h"
#include "net/swap.h"
#include "net/udp.h"
#include "console/console.h"
#include "stdlib/string.h"

// ------------------------------------------------------------------------------------------------
// DHCP Header

typedef struct DHCP_Header
{
    u8 opcode;
    u8 htype;
    u8 hlen;
    u8 hop_count;
    u32 xid;
    u16 sec_count;
    u16 flags;
    IPv4_Addr client_ip_addr;
    IPv4_Addr your_ip_addr;
    IPv4_Addr server_ip_addr;
    IPv4_Addr gateway_ip_addr;
    Eth_Addr client_eth_addr;
    u8 reserved[10];
    char server_name[64];
    char boot_filename[128];
} PACKED DHCP_Header;

// ------------------------------------------------------------------------------------------------
// Opcode

#define OP_REQUEST                      1
#define OP_REPLY                        2

// ------------------------------------------------------------------------------------------------
// Hardware Address Type

#define HTYPE_ETH                       1

// ------------------------------------------------------------------------------------------------
// Options

#define MAGIC_COOKIE                    0x63825363

#define OPT_PAD                         0
#define OPT_SUBNET_MASK                 1
#define OPT_ROUTER                      3
#define OPT_DNS                         6
#define OPT_REQUESTED_IP_ADDR           50
#define OPT_LEASE_TIME                  51
#define OPT_DHCP_MESSAGE_TYPE           53
#define OPT_SERVER_ID                   54
#define OPT_PARAMETER_REQUEST           55
#define OPT_END                         255

typedef struct DHCP_Options
{
    const IPv4_Addr* subnet_mask;
    const IPv4_Addr* router_list;
    const IPv4_Addr* router_end;
    const IPv4_Addr* dns_list;
    const IPv4_Addr* dns_end;
    const IPv4_Addr* requested_ip_addr;
    uint lease_time;
    uint message_type;
    const IPv4_Addr* server_id;
    const u8* parameter_list;
    const u8* parameter_end;
} DHCP_Options;

// ------------------------------------------------------------------------------------------------
// Message Types

#define DHCP_DISCOVER                   1
#define DHCP_OFFER                      2
#define DHCP_REQUEST                    3
#define DHCP_DECLINE                    4
#define DHCP_ACK                        5
#define DHCP_NAK                        6
#define DHCP_RELEASE                    7
#define DHCP_INFORM                     8

// ------------------------------------------------------------------------------------------------
static bool dhcp_parse_options(DHCP_Options* opt, const u8* p, const u8* end)
{
    u32 magic_cookie = net_swap32(*(u32*)p);
    p += 4;

    if (magic_cookie != MAGIC_COOKIE)
    {
        return false;
    }

    memset(opt, 0, sizeof(*opt));

    while (p < end)
    {
        u8 type = *p++;

        if (type == OPT_PAD)
        {
            continue;
        }
        else if (type == OPT_END)
        {
            break;
        }
        else
        {
            u8 opt_len = *p++;

            const u8* next = p + opt_len;
            if (next > end)
            {
                return false;
            }

            switch (type)
            {
            case OPT_SUBNET_MASK:
                opt->subnet_mask = (const IPv4_Addr*)p;
                break;
            case OPT_ROUTER:
                opt->router_list = (const IPv4_Addr*)p;
                opt->router_end = (const IPv4_Addr*)next;
                break;
            case OPT_DNS:
                opt->dns_list = (const IPv4_Addr*)p;
                opt->dns_end = (const IPv4_Addr*)next;
                break;
            case OPT_REQUESTED_IP_ADDR:
                opt->requested_ip_addr = (const IPv4_Addr*)p;
                break;
            case OPT_LEASE_TIME:
                opt->lease_time = net_swap32(*(u32*)p);
                break;
            case OPT_DHCP_MESSAGE_TYPE:
                opt->message_type = *p;
                break;
            case OPT_SERVER_ID:
                opt->server_id = (const IPv4_Addr*)p;
                break;
            case OPT_PARAMETER_REQUEST:
                opt->parameter_list = p;
                opt->parameter_end = next;
                break;
            default:
                console_print("   DHCP: unknown option (%d)\n", type);
                break;
            }

            p = next;
        }
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
static u8* dhcp_build_header(u8* pkt, uint xid, const Eth_Addr* client_eth_addr, u8 message_type)
{
    DHCP_Header* hdr = (DHCP_Header*)pkt;

    memset(hdr, 0, sizeof(DHCP_Header));
    hdr->opcode = OP_REQUEST;
    hdr->htype = HTYPE_ETH;
    hdr->hlen = sizeof(Eth_Addr);
    hdr->hop_count = 0;
    hdr->xid = net_swap32(xid);
    hdr->sec_count = net_swap16(0);
    hdr->flags = net_swap16(0);
    hdr->client_eth_addr = *client_eth_addr;

    // Options
    u8* p = pkt + sizeof(DHCP_Header);

    // Magic Cookie
    *(u32*)p = net_swap32(MAGIC_COOKIE);
    p += 4;

    // DHCP Message Type
    *p++ = OPT_DHCP_MESSAGE_TYPE;
    *p++ = 1;
    *p++ = message_type;

    return p;
}

// ------------------------------------------------------------------------------------------------
static void dhcp_request(Net_Intf* intf, const DHCP_Header* hdr, const DHCP_Options* opt)
{
    uint xid = net_swap32(hdr->xid);
    const IPv4_Addr* requested_ip_addr = &hdr->your_ip_addr;
    const IPv4_Addr* server_id = opt->server_id;

    char requested_ip_addr_str[IPV4_ADDR_STRING_SIZE];
    ipv4_addr_to_str(requested_ip_addr_str, sizeof(requested_ip_addr_str), requested_ip_addr);
    console_print("DHCP requesting lease for %s\n", requested_ip_addr_str);

    NetBuf* buf = net_alloc_packet();
    u8* pkt = (u8*)(buf + 1);

    // Header
    u8* p = dhcp_build_header(pkt, xid, &intf->eth_addr, DHCP_REQUEST);

    // Server Identifier
    *p++ = OPT_SERVER_ID;
    *p++ = sizeof(IPv4_Addr);
    *(IPv4_Addr*)p = *server_id;
    p += sizeof(IPv4_Addr);

    // Requested IP address
    *p++ = OPT_REQUESTED_IP_ADDR;
    *p++ = sizeof(IPv4_Addr);
    *(IPv4_Addr*)p = *requested_ip_addr;
    p += sizeof(IPv4_Addr);

    // Parameter Request list
    *p++ = OPT_PARAMETER_REQUEST;
    *p++ = 3;
    *p++ = OPT_SUBNET_MASK;
    *p++ = OPT_ROUTER;
    *p++ = OPT_DNS;

    // Option End
    *p++ = OPT_END;

    // Send packet
    u8* end = p;
    dhcp_print(pkt, end);
    udp_tx_intf(intf, &broadcast_ipv4_addr, PORT_BOOTP_SERVER, PORT_BOOTP_CLIENT, pkt, end);
}


// ------------------------------------------------------------------------------------------------
static void dhcp_ack(Net_Intf* intf, const DHCP_Header* hdr, const DHCP_Options* opt)
{
    // Update interface IP address
    intf->ip_addr = hdr->your_ip_addr;

    // Add gateway route
    if (opt->router_list)
    {
        net_add_route(&null_ipv4_addr, &null_ipv4_addr, opt->router_list, intf);
    }

    // Add subnet route
    if (opt->subnet_mask)
    {
        IPv4_Addr subnet_addr;
        subnet_addr.u.bits = intf->ip_addr.u.bits & opt->subnet_mask->u.bits;
        net_add_route(&subnet_addr, opt->subnet_mask, 0, intf);
    }

    // Add host route
    IPv4_Addr host_mask = { { { 0xff, 0xff, 0xff, 0xff } } };
    net_add_route(&intf->ip_addr, &host_mask, 0, intf);

    // Record broadcast address
    if (opt->subnet_mask)
    {
        intf->broadcast_addr.u.bits = intf->ip_addr.u.bits | ~opt->subnet_mask->u.bits;
    }

    // Set DNS server
    if (opt->dns_list)
    {
        dns_server = *opt->dns_list;
    }

    // TODO - how to handle ARP with DHCP sequence?
}

// ------------------------------------------------------------------------------------------------
void dhcp_rx(Net_Intf* intf, const u8* pkt, const u8* end)
{
    dhcp_print(pkt, end);

    if (pkt + sizeof(DHCP_Header) > end)
    {
        return;
    }

    const DHCP_Header* hdr = (const DHCP_Header*)pkt;
    if (hdr->opcode != OP_REPLY || hdr->htype != HTYPE_ETH || hdr->hlen != sizeof(Eth_Addr))
    {
        return;
    }

    if (!eth_addr_eq(&intf->eth_addr, &hdr->client_eth_addr))
    {
        return;
    }

    if (end - pkt >= sizeof(DHCP_Header) + 4)
    {
        const u8* p = pkt + sizeof(DHCP_Header);

        DHCP_Options opt;
        if (!dhcp_parse_options(&opt, p, end))
        {
            return;
        }

        char your_ip_addr_str[IPV4_ADDR_STRING_SIZE];
        ipv4_addr_to_str(your_ip_addr_str, sizeof(your_ip_addr_str), &hdr->your_ip_addr);

        switch (opt.message_type)
        {
        case DHCP_OFFER:
            console_print("DHCP offer received for %s\n", your_ip_addr_str);
            dhcp_request(intf, hdr, &opt);
            break;

        case DHCP_ACK:
            console_print("DHCP ack received for %s\n", your_ip_addr_str);
            dhcp_ack(intf, hdr, &opt);
            break;

        case DHCP_NAK:
            console_print("DHCP nak received for %s\n", your_ip_addr_str);
            break;

        default:
            console_print("DHCP message unhandled\n");
            break;
        }
    }
}

// ------------------------------------------------------------------------------------------------
void dhcp_discover(Net_Intf* intf)
{
    console_print("DHCP discovery\n");

    NetBuf* buf = net_alloc_packet();
    u8* pkt = (u8*)(buf + 1);

    // Header
    uint xid = 0;
    u8* p = dhcp_build_header(pkt, xid, &intf->eth_addr, DHCP_DISCOVER);

    // Parameter Request list
    *p++ = OPT_PARAMETER_REQUEST;
    *p++ = 3;
    *p++ = OPT_SUBNET_MASK;
    *p++ = OPT_ROUTER;
    *p++ = OPT_DNS;

    // Option End
    *p++ = OPT_END;

    // Send packet
    u8* end = p;
    dhcp_print(pkt, end);
    udp_tx_intf(intf, &broadcast_ipv4_addr, PORT_BOOTP_SERVER, PORT_BOOTP_CLIENT, pkt, end);
}

// ------------------------------------------------------------------------------------------------
void dhcp_print(const u8* pkt, const u8* end)
{
    if (~net_trace & TRACE_APP)
    {
        return;
    }

    if (pkt + sizeof(DHCP_Header) > end)
    {
        return;
    }

    const DHCP_Header* hdr = (const DHCP_Header*)pkt;

    char client_ip_addr_str[IPV4_ADDR_STRING_SIZE];
    char your_ip_addr_str[IPV4_ADDR_STRING_SIZE];
    char server_ip_addr_str[IPV4_ADDR_STRING_SIZE];
    char gateway_ip_addr_str[IPV4_ADDR_STRING_SIZE];
    char client_eth_addr_str[ETH_ADDR_STRING_SIZE];

    ipv4_addr_to_str(client_ip_addr_str, sizeof(client_ip_addr_str), &hdr->client_ip_addr);
    ipv4_addr_to_str(your_ip_addr_str, sizeof(your_ip_addr_str), &hdr->your_ip_addr);
    ipv4_addr_to_str(server_ip_addr_str, sizeof(server_ip_addr_str), &hdr->server_ip_addr);
    ipv4_addr_to_str(gateway_ip_addr_str, sizeof(gateway_ip_addr_str), &hdr->gateway_ip_addr);
    eth_addr_to_str(client_eth_addr_str, sizeof(client_eth_addr_str), &hdr->client_eth_addr);

    console_print("   DHCP: opcode=%d htype=%d hlen=%d hop_count=%d xid=%d secs=%d flags=%d len=%d\n",
        hdr->opcode, hdr->htype, hdr->hlen, hdr->hop_count,
        net_swap32(hdr->xid), net_swap16(hdr->sec_count), net_swap16(hdr->flags), end - pkt);
    console_print("   DHCP: client=%s your=%s server=%s gateway=%s\n",
        client_ip_addr_str, your_ip_addr_str, server_ip_addr_str, gateway_ip_addr_str);
    console_print("   DHCP: eth=%s server_name=%s boot_filename=%s\n",
        client_eth_addr_str, hdr->server_name, hdr->boot_filename);

    if (end - pkt >= sizeof(DHCP_Header) + 4)
    {
        const u8* p = pkt + sizeof(DHCP_Header);

        DHCP_Options opt;
        if (!dhcp_parse_options(&opt, p, end))
        {
            return;
        }

        char ipv4_addr_str[IPV4_ADDR_STRING_SIZE];

        if (opt.message_type)
        {
            console_print("   DHCP: message type: %d\n", opt.message_type);
        }

        if (opt.subnet_mask)
        {
            ipv4_addr_to_str(ipv4_addr_str, sizeof(ipv4_addr_str), opt.subnet_mask);
            console_print("   DHCP: subnet_mask: %s\n", ipv4_addr_str);
        }

        for (const IPv4_Addr* addr = opt.router_list; addr != opt.router_end; ++addr)
        {
            ipv4_addr_to_str(ipv4_addr_str, sizeof(ipv4_addr_str), addr);
            console_print("   DHCP: router: %s\n", ipv4_addr_str);
        }

        for (const IPv4_Addr* addr = opt.dns_list; addr != opt.dns_end; ++addr)
        {
            ipv4_addr_to_str(ipv4_addr_str, sizeof(ipv4_addr_str), addr);
            console_print("   DHCP: dns: %s\n", ipv4_addr_str);
        }

        if (opt.requested_ip_addr)
        {
            ipv4_addr_to_str(ipv4_addr_str, sizeof(ipv4_addr_str), opt.requested_ip_addr);
            console_print("   DHCP: requested ip: %s\n", ipv4_addr_str);
        }

        if (opt.lease_time)
        {
            console_print("   DHCP: lease time: %d\n", opt.lease_time);
        }

        if (opt.server_id)
        {
            ipv4_addr_to_str(ipv4_addr_str, sizeof(ipv4_addr_str), opt.server_id);
            console_print("   DHCP: server id: %s\n", ipv4_addr_str);
        }

        for (const u8* p = opt.parameter_list; p != opt.parameter_end; ++p)
        {
            console_print("   DHCP: parameter request: %d\n", *p);
        }
    }
}
