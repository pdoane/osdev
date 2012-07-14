// ------------------------------------------------------------------------------------------------
// net/tcp.c
// ------------------------------------------------------------------------------------------------

#include "net/tcp.h"
#include "net/buf.h"
#include "net/checksum.h"
#include "net/ipv4.h"
#include "net/net.h"
#include "net/port.h"
#include "net/route.h"
#include "net/swap.h"
#include "console/console.h"
#include "mem/vm.h"
#include "stdlib/string.h"
#include "time/pit.h"
#include "time/rtc.h"

// ------------------------------------------------------------------------------------------------
// Static Variables

static u32 tcp_base_isn;
static Link tcp_free_conns = { &tcp_free_conns, &tcp_free_conns };
static Link tcp_active_conns = { &tcp_active_conns, &tcp_active_conns};

// ------------------------------------------------------------------------------------------------
static bool tcp_parse_options(TCP_Options* opt, const u8* p, const u8* end)
{
    memset(opt, 0, sizeof(*opt));

    while (p < end)
    {
        u8 type = *p++;

        if (type == OPT_NOP)
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

            const u8* next = p + opt_len - 2;
            if (next > end)
            {
                return false;
            }

            switch (type)
            {
            case OPT_MSS:
                opt->mss = net_swap16(*(u16*)p);
                break;
            }

            p = next;
        }
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
static void tcp_print(const u8* pkt, const u8* end)
{
    /*
    if (!net_trace)
    {
        return;
    }*/

    if (pkt + sizeof(TCP_Header) > end)
    {
        return;
    }

    Checksum_Header* phdr = (Checksum_Header*)(pkt - sizeof(Checksum_Header));
    char src_addr_str[IPV4_ADDR_STRING_SIZE];
    char dst_addr_str[IPV4_ADDR_STRING_SIZE];
    ipv4_addr_to_str(src_addr_str, sizeof(src_addr_str), &phdr->src);
    ipv4_addr_to_str(dst_addr_str, sizeof(dst_addr_str), &phdr->dst);

    const TCP_Header* hdr = (const TCP_Header*)pkt;

    u16 src_port = net_swap16(hdr->src_port);
    u16 dst_port = net_swap16(hdr->dst_port);
    u32 seq = net_swap32(hdr->seq);
    u32 ack = net_swap32(hdr->ack);
    u16 window_size = net_swap16(hdr->window_size);
    u16 checksum = net_swap16(hdr->checksum);
    u16 urgent = net_swap16(hdr->urgent);

    u16 checksum2 = net_checksum(pkt - sizeof(Checksum_Header), end);

    uint hdr_len = hdr->off >> 2;
    //const u8* data = (pkt + hdr_len);
    uint data_len = (end - pkt) - hdr_len;

    console_print("  TCP: src=%s:%d dst=%s:%d\n",
            src_addr_str, src_port, dst_addr_str, dst_port);
    console_print("  TCP: seq=%u ack=%u data_len=%u\n",
            seq, ack, data_len);
    console_print("  TCP: flags=%02x window=%u urgent=%u checksum=%u%c\n",
            hdr->flags, window_size, urgent, checksum,
            checksum2 ? '!' : ' ');

    if (hdr_len > sizeof(TCP_Header))
    {
        const u8* p = pkt + sizeof(TCP_Header);
        const u8* end = p + hdr_len;

        TCP_Options opt;
        tcp_parse_options(&opt, p, end);

        if (opt.mss)
        {
            console_print("  TCP: mss=%u\n", opt.mss);
        }
    }
}

// ------------------------------------------------------------------------------------------------
static void tcp_tx(TCP_Conn* conn, u32 seq, u32 ack, u8 flags)
{
    NetBuf* buf = net_alloc_packet();
    u8* pkt = (u8*)(buf + 1);

    // Header
    TCP_Header* hdr = (TCP_Header*)pkt;
    hdr->src_port = conn->local_port;
    hdr->dst_port = conn->remote_port;
    hdr->seq = seq;
    hdr->ack = ack;
    hdr->off = 0;
    hdr->flags = flags;
    hdr->window_size = TCP_WINDOW_SIZE;
    hdr->checksum = 0;
    hdr->urgent = 0;
    tcp_swap(hdr);

    u8* p = pkt + sizeof(TCP_Header);

    if (flags & TCP_SYN)
    {
        // Maximum Segment Size
        p[0] = OPT_MSS;
        p[1] = 4;
        *(u16*)(p + 2) = net_swap16(1460);
        p += p[1];
    }

    // Option End
    while ((p - pkt) & 3)
    {
        *p++ = 0;
    }

    hdr->off = (p - pkt) << 2;

    // Data
    u8* end = p;

    // Pseudo Header
    Checksum_Header* phdr = (Checksum_Header*)(pkt - sizeof(Checksum_Header));
    phdr->src = conn->local_addr;
    phdr->dst = conn->remote_addr;
    phdr->reserved = 0;
    phdr->protocol = IP_PROTOCOL_TCP;
    phdr->len = net_swap16(end - pkt);

    // Checksum
    u16 checksum = net_checksum(pkt - sizeof(Checksum_Header), end);
    hdr->checksum = net_swap16(checksum);

    // Transmit
    tcp_print(pkt, end);
    ipv4_tx_intf(conn->intf, &conn->next_addr, &conn->remote_addr, IP_PROTOCOL_TCP, pkt, end);
}

// ------------------------------------------------------------------------------------------------
static TCP_Conn* tcp_find(const IPv4_Addr* src_addr, u16 src_port,
    const IPv4_Addr* dst_addr, u16 dst_port)
{
    TCP_Conn* conn;
    list_for_each(conn, tcp_active_conns, link)
    {
        if (src_port == conn->remote_port &&
            dst_port == conn->local_port &&
            ipv4_addr_eq(src_addr, &conn->remote_addr) &&
            ipv4_addr_eq(dst_addr, &conn->local_addr))
        {
            return conn;
        }
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
static void tcp_error(TCP_Conn* conn, const char* msg)
{
    if (conn->on_error)
    {
        conn->on_error(msg);
    }

    tcp_close(conn);
}

// ------------------------------------------------------------------------------------------------
void tcp_init()
{
    // Compute base ISN from system clock and ticks since boot.  ISN is incremented every 4 us.
    DateTime dt;
    rtc_get_time(&dt);
    abs_time t = join_time(&dt);

    tcp_base_isn = (t * 1000 - pit_ticks) * 250;
}

// ------------------------------------------------------------------------------------------------
static void tcp_rx_closed(Checksum_Header* phdr, TCP_Header* hdr)
{
    console_print("TCP: packet received in CLOSED state\n");

    // Drop packet if this is a RST
    if (hdr->flags & TCP_RST)
    {
        return;
    }

    // Find an appropriate interface to route packet
    const IPv4_Addr* dst_addr = &phdr->src;
    const Net_Route* route = net_find_route(dst_addr);

    if (!route)
    {
        return;
    }

    // Create dummy connection for sending RST
    TCP_Conn rst_conn;
    memset(&rst_conn, 0, sizeof(TCP_Conn));

    rst_conn.intf = route->intf;
    rst_conn.local_addr = phdr->dst;
    rst_conn.local_port = hdr->dst_port;
    rst_conn.remote_addr = phdr->src;
    rst_conn.remote_port = hdr->src_port;
    rst_conn.next_addr = *net_next_addr(route, dst_addr);

    if (hdr->flags & TCP_ACK)
    {
        tcp_tx(&rst_conn, hdr->ack, 0, TCP_RST);
    }
    else
    {
        uint hdr_len = hdr->off >> 2;
        uint data_len = phdr->len - hdr_len;
        tcp_tx(&rst_conn, 0, hdr->seq + data_len, TCP_RST | TCP_ACK);
    }
}

// ------------------------------------------------------------------------------------------------
static void tcp_rx_syn_sent(TCP_Conn* conn, Checksum_Header* phdr, TCP_Header* hdr)
{
    uint flags = hdr->flags;

    if (flags & TCP_ACK)
    {
        // Early out for bad ACK
        if (SEQ_LE(hdr->ack, conn->iss) || SEQ_GT(hdr->ack, conn->snd_nxt))
        {
            if (~flags & TCP_RST)
            {
                console_print("TCP: Bad ACK received\n");
                tcp_tx(conn, hdr->ack, 0, TCP_RST);
            }

            return;
        }
    }

    if (flags & TCP_RST)
    {
        // Process RST
        if (flags & TCP_ACK)
        {
            tcp_error(conn, "connection reset");
        }
    }
    else if (flags & TCP_SYN)
    {
        // SYN is set.  ACK is either ok or there was no ACK.  No RST.

        conn->irs = hdr->seq;
        conn->rcv_nxt = hdr->seq + 1;

        if (flags & TCP_ACK)
        {
            conn->snd_una = hdr->ack;

            // TODO - Segments on the retransmission queue which are ack'd should be removed
        }

        if (SEQ_GT(conn->snd_una, conn->iss))
        {
            console_print("TCP: Connection Established\n");

            conn->state = TCP_ESTABLISHED;
            tcp_tx(conn, conn->snd_nxt, conn->rcv_nxt, TCP_ACK);

            // TODO - Data queued for transmission may be included with the ACK.

            // TODO - If there is data in the segment, continue processing at the URG phase.
        }
        else
        {
            conn->state = TCP_SYN_RECEIVED;
            tcp_tx(conn, conn->iss, conn->rcv_nxt, TCP_SYN | TCP_ACK);
        }
    }
}

// ------------------------------------------------------------------------------------------------
void tcp_rx(Net_Intf* intf, u8* pkt, u8* end)
{
    // Validate packet header
    const IPv4_Header* ip_hdr = (const IPv4_Header*)pkt;

    uint ihl = (ip_hdr->ver_ihl) & 0xf;
    const u8* tcp_pkt = pkt + (ihl << 2);

    if (pkt + sizeof(TCP_Header) > end)
    {
        return;
    }

    // Assemble Pseudo Header
    IPv4_Addr src_addr = ip_hdr->src;
    IPv4_Addr dst_addr = ip_hdr->dst;
    u8 protocol = ip_hdr->protocol;

    Checksum_Header* phdr = (Checksum_Header*)(tcp_pkt - sizeof(Checksum_Header));
    phdr->src = src_addr;
    phdr->dst = dst_addr;
    phdr->reserved = 0;
    phdr->protocol = protocol;
    phdr->len = net_swap16(end - tcp_pkt);

    tcp_print(tcp_pkt, end);

    // Validate checksum
    if (net_checksum(tcp_pkt - sizeof(Checksum_Header), end))
    {
        console_print("TCP: Dropping packet with bad checksum\n");
        return;
    }

    // Process packet
    TCP_Header* hdr = (TCP_Header*)tcp_pkt;
    tcp_swap(hdr);
    phdr->len = net_swap16(phdr->len);

    // Find connection associated with packet
    TCP_Conn* conn = tcp_find(&phdr->src, hdr->src_port, &phdr->dst, hdr->dst_port);
    if (!conn || conn->state == TCP_CLOSED)
    {
        tcp_rx_closed(phdr, hdr);
        return;
    }

    // Process packet by state
    if (conn->state == TCP_LISTEN)
    {
    }
    else if (conn->state == TCP_SYN_SENT)
    {
        tcp_rx_syn_sent(conn, phdr, hdr);
    }
    else
    {
    }
}

// ------------------------------------------------------------------------------------------------
void tcp_swap(TCP_Header* hdr)
{
    hdr->src_port = net_swap16(hdr->src_port);
    hdr->dst_port = net_swap16(hdr->dst_port);
    hdr->seq = net_swap32(hdr->seq);
    hdr->ack = net_swap32(hdr->ack);
    hdr->window_size = net_swap16(hdr->window_size);
    hdr->checksum = net_swap16(hdr->checksum);
    hdr->urgent = net_swap16(hdr->urgent);
}

// ------------------------------------------------------------------------------------------------
TCP_Conn* tcp_create()
{
    Link* p = tcp_free_conns.next;
    if (p != &tcp_free_conns)
    {
        link_remove(p);
        return link_data(p, TCP_Conn, link);
    }
    else
    {
        TCP_Conn* conn = vm_alloc(sizeof(TCP_Conn));
        memset(conn, 0, sizeof(TCP_Conn));
        return conn;
    }
}

// ------------------------------------------------------------------------------------------------
bool tcp_connect(TCP_Conn* conn, const IPv4_Addr* addr, u16 port)
{
    // Find network interface through the routing table.
    const Net_Route* route = net_find_route(addr);
    if (!route)
    {
        return false;
    }

    Net_Intf* intf = route->intf;

    // Initialize connection
    conn->intf = intf;
    conn->local_addr = intf->ip_addr;
    conn->next_addr = *net_next_addr(route, addr);
    conn->remote_addr = *addr;
    conn->local_port = net_ephemeral_port();
    conn->remote_port = port;

    u32 isn = tcp_base_isn + pit_ticks * 250;

    conn->snd_una = isn;
    conn->snd_nxt = isn + 1;
    conn->snd_wnd = TCP_WINDOW_SIZE;
    conn->snd_up = 0;
    conn->snd_wl1 = 0;
    conn->snd_wl2 = 0;
    conn->iss = isn;

    conn->rcv_nxt = 0;
    conn->rcv_wnd = TCP_WINDOW_SIZE;
    conn->rcv_up = 0;
    conn->irs = 0;

    // Link to active connections
    link_before(&tcp_active_conns, &conn->link);

    // Issue SYN segment
    tcp_tx(conn, isn, 0, TCP_SYN);
    conn->state = TCP_SYN_SENT;

    return true;
}

// ------------------------------------------------------------------------------------------------
void tcp_close(TCP_Conn* conn)
{
    link_before(&tcp_free_conns, &conn->link);
}

// ------------------------------------------------------------------------------------------------
void tcp_send(TCP_Conn* conn, const void* data, uint count)
{
}

// ------------------------------------------------------------------------------------------------
uint tcp_recv(TCP_Conn* conn, void* data, uint count)
{
    return 0;
}
