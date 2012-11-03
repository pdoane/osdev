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
// Static/Global Variables

static u32 tcp_base_isn;
static Link tcp_free_conns = { &tcp_free_conns, &tcp_free_conns };

Link tcp_active_conns = { &tcp_active_conns, &tcp_active_conns};

// ------------------------------------------------------------------------------------------------
// TCP state strings

const char* tcp_state_strs[] =
{
    "CLOSED",
    "LISTEN",
    "SYN-SENT",
    "SYN-RECEIVED",
    "ESTABLISHED",
    "FIN-WAIT-1",
    "FIN-WAIT-2",
    "CLOSE-WAIT",
    "CLOSING",
    "LAST-ACK",
    "TIME-WAIT"
};

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

            if (opt_len < 2)
            {
                return false;
            }

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
static void tcp_print(const Net_Buf* pkt)
{
    if (~net_trace & TRACE_TRANSPORT)
    {
        return;
    }

    if (pkt->start + sizeof(TCP_Header) > pkt->end)
    {
        return;
    }

    Checksum_Header* phdr = (Checksum_Header*)(pkt->start - sizeof(Checksum_Header));
    char src_addr_str[IPV4_ADDR_STRING_SIZE];
    char dst_addr_str[IPV4_ADDR_STRING_SIZE];
    ipv4_addr_to_str(src_addr_str, sizeof(src_addr_str), &phdr->src);
    ipv4_addr_to_str(dst_addr_str, sizeof(dst_addr_str), &phdr->dst);

    const TCP_Header* hdr = (const TCP_Header*)pkt->start;

    u16 src_port = net_swap16(hdr->src_port);
    u16 dst_port = net_swap16(hdr->dst_port);
    u32 seq = net_swap32(hdr->seq);
    u32 ack = net_swap32(hdr->ack);
    u16 window_size = net_swap16(hdr->window_size);
    u16 checksum = net_swap16(hdr->checksum);
    u16 urgent = net_swap16(hdr->urgent);

    u16 checksum2 = net_checksum(pkt->start - sizeof(Checksum_Header), pkt->end);

    uint hdr_len = hdr->off >> 2;
    //const u8* data = (pkt->start + hdr_len);
    uint data_len = (pkt->end - pkt->start) - hdr_len;

    console_print("  TCP: src=%s:%d dst=%s:%d\n",
            src_addr_str, src_port, dst_addr_str, dst_port);
    console_print("  TCP: seq=%u ack=%u data_len=%u\n",
            seq, ack, data_len);
    console_print("  TCP: flags=%02x window=%u urgent=%u checksum=%u%c\n",
            hdr->flags, window_size, urgent, checksum,
            checksum2 ? '!' : ' ');

    if (hdr_len > sizeof(TCP_Header))
    {
        const u8* p = pkt->start + sizeof(TCP_Header);
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
static void tcp_set_state(TCP_Conn* conn, uint state)
{
    uint old_state = conn->state;
    conn->state = state;

    if (conn->on_state)
    {
        conn->on_state(conn, old_state, state);
    }
}

// ------------------------------------------------------------------------------------------------
static TCP_Conn* tcp_alloc()
{
    Link* p = tcp_free_conns.next;
    if (p != &tcp_free_conns)
    {
        link_remove(p);
        return link_data(p, TCP_Conn, link);
    }
    else
    {
        return vm_alloc(sizeof(TCP_Conn));
    }
}

// ------------------------------------------------------------------------------------------------
static void tcp_free(TCP_Conn* conn)
{
    if (conn->state != TCP_CLOSED)
    {
        tcp_set_state(conn, TCP_CLOSED);
    }

    Net_Buf* pkt;
    Net_Buf* next;
    list_for_each_safe(pkt, next, conn->resequence, link)
    {
        link_remove(&pkt->link);
        net_release_buf(pkt);
    }

    link_move_before(&tcp_free_conns, &conn->link);
}

// ------------------------------------------------------------------------------------------------
static void tcp_tx(TCP_Conn* conn, u32 seq, u8 flags, const void* data, uint count)
{
    Net_Buf* pkt = net_alloc_buf();

    // Header
    TCP_Header* hdr = (TCP_Header*)pkt->start;
    hdr->src_port = conn->local_port;
    hdr->dst_port = conn->remote_port;
    hdr->seq = seq;
    hdr->ack = flags & TCP_ACK ? conn->rcv_nxt : 0;
    hdr->off = 0;
    hdr->flags = flags;
    hdr->window_size = TCP_WINDOW_SIZE;
    hdr->checksum = 0;
    hdr->urgent = 0;
    tcp_swap(hdr);

    u8* p = pkt->start + sizeof(TCP_Header);

    if (flags & TCP_SYN)
    {
        // Maximum Segment Size
        p[0] = OPT_MSS;
        p[1] = 4;
        *(u16*)(p + 2) = net_swap16(1460);
        p += p[1];
    }

    // Option End
    while ((p - pkt->start) & 3)
    {
        *p++ = 0;
    }

    hdr->off = (p - pkt->start) << 2;

    // Data
    memcpy(p, data, count);
    pkt->end = p + count;

    // Pseudo Header
    Checksum_Header* phdr = (Checksum_Header*)(pkt->start - sizeof(Checksum_Header));
    phdr->src = conn->local_addr;
    phdr->dst = conn->remote_addr;
    phdr->reserved = 0;
    phdr->protocol = IP_PROTOCOL_TCP;
    phdr->len = net_swap16(pkt->end - pkt->start);

    // Checksum
    u16 checksum = net_checksum(pkt->start - sizeof(Checksum_Header), pkt->end);
    hdr->checksum = net_swap16(checksum);

    // Transmit
    tcp_print(pkt);
    ipv4_tx_intf(conn->intf, &conn->next_addr, &conn->remote_addr, IP_PROTOCOL_TCP, pkt);

    // Update State
    conn->snd_nxt += count;
    if (flags & (TCP_SYN | TCP_FIN))
    {
        ++conn->snd_nxt;
    }
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
static void tcp_error(TCP_Conn* conn, uint error)
{
    if (conn->on_error)
    {
        conn->on_error(conn, error);
    }

    tcp_free(conn);
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
        tcp_tx(&rst_conn, hdr->ack, TCP_RST, 0, 0);
    }
    else
    {
        uint hdr_len = hdr->off >> 2;
        uint data_len = phdr->len - hdr_len;

        rst_conn.rcv_nxt = hdr->seq + data_len;

        tcp_tx(&rst_conn, 0, TCP_RST | TCP_ACK, 0, 0);
    }
}

// ------------------------------------------------------------------------------------------------
static void tcp_rx_syn_sent(TCP_Conn* conn, TCP_Header* hdr)
{
    uint flags = hdr->flags;

    // Check for bad ACK first.
    if (flags & TCP_ACK)
    {
        if (SEQ_LE(hdr->ack, conn->iss) || SEQ_GT(hdr->ack, conn->snd_nxt))
        {
            if (~flags & TCP_RST)
            {
                tcp_tx(conn, hdr->ack, TCP_RST, 0, 0);
            }

            return;
        }
    }

    // Check for RST
    if (flags & TCP_RST)
    {
        if (flags & TCP_ACK)
        {
            tcp_error(conn, TCP_CONN_RESET);
        }

        return;
    }

    // Check SYN
    if (flags & TCP_SYN)
    {
        // SYN is set.  ACK is either ok or there was no ACK.  No RST.

        conn->irs = hdr->seq;
        conn->rcv_nxt = hdr->seq + 1;

        if (flags & TCP_ACK)
        {
            conn->snd_una = hdr->ack;
            conn->snd_wnd = hdr->window_size;
            conn->snd_wl1 = hdr->seq;
            conn->snd_wl2 = hdr->ack;

            // TODO - Segments on the retransmission queue which are ack'd should be removed

            tcp_set_state(conn, TCP_ESTABLISHED);
            tcp_tx(conn, conn->snd_nxt, TCP_ACK, 0, 0);


            // TODO - Data queued for transmission may be included with the ACK.

            // TODO - If there is data in the segment, continue processing at the URG phase.
        }
        else
        {
            tcp_set_state(conn, TCP_SYN_RECEIVED);

            // Resend ISS
            --conn->snd_nxt;
            tcp_tx(conn, conn->snd_nxt, TCP_SYN | TCP_ACK, 0, 0);
        }
    }
}

// ------------------------------------------------------------------------------------------------
static void tcp_rx_rst(TCP_Conn* conn, TCP_Header* hdr)
{
    switch (conn->state)
    {
    case TCP_SYN_RECEIVED:
        // TODO - All segments on the retransmission queue should be removed

        // TODO - If initiated with a passive open, go to LISTEN state

        tcp_error(conn, TCP_CONN_REFUSED);
        break;

    case TCP_ESTABLISHED:
    case TCP_FIN_WAIT_1:
    case TCP_FIN_WAIT_2:
    case TCP_CLOSE_WAIT:
        // TODO - All outstanding sends should receive "reset" responses

        tcp_error(conn, TCP_CONN_RESET);
        break;

    case TCP_CLOSING:
    case TCP_LAST_ACK:
    case TCP_TIME_WAIT:
        tcp_free(conn);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
static void tcp_rx_syn(TCP_Conn* conn, TCP_Header* hdr)
{
    // TODO - All outstanding sends should receive "reset" responses

    tcp_tx(conn, 0, TCP_RST | TCP_ACK, 0, 0);

    tcp_error(conn, TCP_CONN_RESET);
}

// ------------------------------------------------------------------------------------------------
static void tcp_rx_ack(TCP_Conn* conn, TCP_Header* hdr)
{
    switch (conn->state)
    {
    case TCP_SYN_RECEIVED:
        if (conn->snd_una <= hdr->ack && hdr->ack <= conn->snd_nxt)
        {
            conn->snd_wnd = hdr->window_size;
            conn->snd_wl1 = hdr->seq;
            conn->snd_wl2 = hdr->ack;
            tcp_set_state(conn, TCP_ESTABLISHED);
        }
        else
        {
            tcp_tx(conn, hdr->ack, TCP_RST, 0, 0);
        }
        break;

    case TCP_ESTABLISHED:
    case TCP_FIN_WAIT_1:
    case TCP_FIN_WAIT_2:
    case TCP_CLOSE_WAIT:
    case TCP_CLOSING:
        // Handle expected acks
        if (SEQ_LE(conn->snd_una, hdr->ack) && SEQ_LE(hdr->ack, conn->snd_nxt))
        {
            // Update acknowledged pointer
            conn->snd_una = hdr->ack;

            // Update send window
            if (SEQ_LT(conn->snd_wl1, hdr->seq) ||
                (conn->snd_wl1 == hdr->seq && SEQ_LE(conn->snd_wl2, hdr->ack)))
            {
                conn->snd_wnd = hdr->window_size;
                conn->snd_wl1 = hdr->seq;
                conn->snd_wl2 = hdr->ack;
            }

            // TODO - remove segments on the retransmission queue which have been ack'd
            // TODO - acknowledge buffers which have sent to user
        }

        // Check for duplicate ack
        if (SEQ_LE(hdr->ack, conn->snd_una))
        {
            // TODO - anything to do here?
        }

        // Check for ack of unsent data
        if (SEQ_GT(hdr->ack, conn->snd_nxt))
        {
            tcp_tx(conn, conn->snd_nxt, TCP_ACK, 0, 0);
            return;
        }

        // Check for ack of FIN
        if (SEQ_GE(hdr->ack, conn->snd_nxt))
        {
            // TODO - is this the right way to detect that our FIN has been ACK'd?
            if (conn->state == TCP_FIN_WAIT_1)
            {
                tcp_set_state(conn, TCP_FIN_WAIT_2);
            }
            else if (conn->state == TCP_CLOSING)
            {
                tcp_set_state(conn, TCP_TIME_WAIT);
                conn->msl_wait = pit_ticks + 2 * TCP_MSL;
            }
        }

        break;

    case TCP_LAST_ACK:
        // Check for ack of FIN
        if (SEQ_GE(hdr->ack, conn->snd_nxt))
        {
            // TODO - is this the right way to detect that our FIN has been ACK'd?

            tcp_free(conn);
        }
        break;

    case TCP_TIME_WAIT:
        // This case is handled in the FIN processing step.
        break;
    }
}

// ------------------------------------------------------------------------------------------------
static void tcp_rx_insert(TCP_Conn* conn, Net_Buf* pkt)
{
    Net_Buf* prev;
    Net_Buf* cur;
    Net_Buf* next;

    uint data_len = pkt->end - pkt->start;
    uint pkt_end = pkt->seq + data_len;

    // Find location to insert packet
    list_for_each(cur, conn->resequence, link)
    {
        if (SEQ_LE(pkt->seq, cur->seq))
        {
            break;
        }
    }

    // Check if we already have some of this data in the previous packet.
    if (cur->link.prev != &conn->resequence)
    {
        prev = link_data(cur->link.prev, Net_Buf, link);
        uint prev_end = prev->seq + prev->end - prev->start;

        if (SEQ_GE(prev_end, pkt_end))
        {
            // Complete overlap with queued packet - drop incoming packet
            net_release_buf(pkt);
            return;
        }
        else if (SEQ_GT(prev_end, pkt->seq))
        {
            // Trim previous packet by overlap with this packet
            prev->end -= prev_end - pkt->seq;
        }
    }

    // Remove all later packets if a FIN has been received
    if (pkt->flags & TCP_FIN)
    {
        while (&cur->link != &conn->resequence)
        {
            next = link_data(cur->link.next, Net_Buf, link);
            link_remove(&cur->link);
            net_release_buf(cur);
            cur = next;
        }
    }

    // Trim/remove later packets that overlap
    while (&cur->link != &conn->resequence)
    {
        uint pkt_end = pkt->seq + data_len;
        uint cur_end = cur->seq + cur->end - cur->start;

        if (SEQ_LT(pkt_end, cur->seq))
        {
            // No overlap
            break;
        }

        if (SEQ_LT(pkt_end, cur_end))
        {
            // Partial overlap - trim
            pkt->end -= pkt_end - cur->seq;
            break;
        }

        // Complete overlap - remove
        next = link_data(cur->link.next, Net_Buf, link);
        link_remove(&cur->link);
        net_release_buf(cur);
        cur = next;
    }

    // Add packet to the queue
    link_before(&cur->link, &pkt->link);
}

// ------------------------------------------------------------------------------------------------
static void tcp_rx_process(TCP_Conn* conn)
{
    Net_Buf* pkt;
    Net_Buf* next;
    list_for_each_safe(pkt, next, conn->resequence, link)
    {
        if (conn->rcv_nxt != pkt->seq)
        {
            break;
        }

        uint data_len = pkt->end - pkt->start;
        conn->rcv_nxt += data_len;

        if (conn->on_data)
        {
            conn->on_data(conn, pkt->start, data_len);
        }

        link_remove(&pkt->link);
        net_release_buf(pkt);
    }
}

// ------------------------------------------------------------------------------------------------
static void tcp_rx_data(TCP_Conn* conn, Net_Buf* pkt)
{
    switch (conn->state)
    {
    case TCP_SYN_RECEIVED:
        // TODO - can this happen? ACK processing would transition to ESTABLISHED state.
        break;

    case TCP_ESTABLISHED:
    case TCP_FIN_WAIT_1:
    case TCP_FIN_WAIT_2:
        // Increase ref count on packet
        ++pkt->ref_count;

        // Insert packet on to input queue sorted by sequence
        tcp_rx_insert(conn, pkt);

        // Process packets that are now in order
        tcp_rx_process(conn);

        // Acknowledge receipt of data
        tcp_tx(conn, conn->snd_nxt, TCP_ACK, 0, 0);
        break;

    default:
        // FIN has been received from the remote side - ignore the segment data.
        break;
    }
}

// ------------------------------------------------------------------------------------------------
static void tcp_rx_fin(TCP_Conn* conn, TCP_Header* hdr)
{
    // TODO - signal the user "connection closing" and return any pending receives

    conn->rcv_nxt = hdr->seq + 1;
    tcp_tx(conn, conn->snd_nxt, TCP_ACK, 0, 0);

    switch (conn->state)
    {
    case TCP_SYN_RECEIVED:
    case TCP_ESTABLISHED:
        tcp_set_state(conn, TCP_CLOSE_WAIT);
        break;

    case TCP_FIN_WAIT_1:
        if (SEQ_GE(hdr->ack, conn->snd_nxt))
        {
            // TODO - is this the right way to detect that our FIN has been ACK'd?

            // TODO - turn off the other timers
            tcp_set_state(conn, TCP_TIME_WAIT);
            conn->msl_wait = pit_ticks + 2 * TCP_MSL;
        }
        else
        {
            tcp_set_state(conn, TCP_CLOSING);
        }
        break;

    case TCP_FIN_WAIT_2:
        // TODO - turn off the other timers
        tcp_set_state(conn, TCP_TIME_WAIT);
        conn->msl_wait = pit_ticks + 2 * TCP_MSL;
        break;

    case TCP_CLOSE_WAIT:
    case TCP_CLOSING:
    case TCP_LAST_ACK:
        break;

    case TCP_TIME_WAIT:
        conn->msl_wait = pit_ticks + 2 * TCP_MSL;
        break;
    }
}

// ------------------------------------------------------------------------------------------------
static void tcp_rx_general(TCP_Conn* conn, TCP_Header* hdr, Net_Buf* pkt)
{
    // Process segments not in the CLOSED, LISTEN, or SYN-SENT states.

    uint flags = hdr->flags;
    uint data_len = pkt->end - pkt->start;

    // Check that sequence and segment data is acceptable
    if (!(SEQ_LE(conn->rcv_nxt, hdr->seq) && SEQ_LE(hdr->seq + data_len, conn->rcv_nxt + conn->rcv_wnd)))
    {
        // Unacceptable segment
        if (~flags & TCP_RST)
        {
            tcp_tx(conn, conn->snd_nxt, TCP_ACK, 0, 0);
        }

        return;
    }

    // TODO - trim segment data?

    // Check RST bit
    if (flags & TCP_RST)
    {
        tcp_rx_rst(conn, hdr);
        return;
    }

    // Check SYN bit
    if (flags & TCP_SYN)
    {
        tcp_rx_syn(conn, hdr);
    }

    // Check ACK
    if (~flags & TCP_ACK)
    {
        return;
    }

    tcp_rx_ack(conn, hdr);

    // TODO - check URG

    // Process segment data
    if (data_len)
    {
        tcp_rx_data(conn, pkt);
    }

    // Check FIN - TODO, needs to handle out of sequence
    if (flags & TCP_FIN)
    {
        tcp_rx_fin(conn, hdr);
    }
}

// ------------------------------------------------------------------------------------------------
void tcp_rx(Net_Intf* intf, const IPv4_Header* ip_hdr, Net_Buf* pkt)
{
    // Validate packet header
    if (pkt->start + sizeof(TCP_Header) > pkt->end)
    {
        return;
    }

    // Assemble Pseudo Header
    IPv4_Addr src_addr = ip_hdr->src;
    IPv4_Addr dst_addr = ip_hdr->dst;
    u8 protocol = ip_hdr->protocol;

    Checksum_Header* phdr = (Checksum_Header*)(pkt->start - sizeof(Checksum_Header));
    phdr->src = src_addr;
    phdr->dst = dst_addr;
    phdr->reserved = 0;
    phdr->protocol = protocol;
    phdr->len = net_swap16(pkt->end - pkt->start);

    tcp_print(pkt);

    // Validate checksum
    if (net_checksum(pkt->start - sizeof(Checksum_Header), pkt->end))
    {
        return;
    }

    // Process packet
    TCP_Header* hdr = (TCP_Header*)pkt->start;
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
        tcp_rx_syn_sent(conn, hdr);
    }
    else
    {
        // Update packet to point to data, and store parts of
        // header needed for out of order handling.
        uint hdr_len = hdr->off >> 2;
        pkt->start += hdr_len;
        pkt->seq = hdr->seq;
        pkt->flags = hdr->flags;

        tcp_rx_general(conn, hdr, pkt);
    }
}

// ------------------------------------------------------------------------------------------------
void tcp_poll()
{
    TCP_Conn* conn;
    TCP_Conn* next;
    list_for_each_safe(conn, next, tcp_active_conns, link)
    {
        if (conn->state == TCP_TIME_WAIT && SEQ_GE(pit_ticks, conn->msl_wait))
        {
            tcp_free(conn);
        }
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
    TCP_Conn* conn = tcp_alloc();
    memset(conn, 0, sizeof(TCP_Conn));
    conn->resequence.next = &conn->resequence;
    conn->resequence.prev = &conn->resequence;

    return conn;
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
    conn->snd_nxt = isn;
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
    tcp_tx(conn, conn->snd_nxt, TCP_SYN, 0, 0);
    tcp_set_state(conn, TCP_SYN_SENT);

    return true;
}

// ------------------------------------------------------------------------------------------------
void tcp_close(TCP_Conn* conn)
{
    switch (conn->state)
    {
    case TCP_CLOSED:
        tcp_free(conn);
        break;

    case TCP_LISTEN:
        tcp_free(conn);
        break;

    case TCP_SYN_SENT:
        // TODO - cancel queued sends
        tcp_free(conn);
        break;

    case TCP_SYN_RECEIVED:
        // TODO - if sends have been issued or queued, wait for ESTABLISHED
        // before entering FIN-WAIT-1
        tcp_tx(conn, conn->snd_nxt, TCP_FIN | TCP_ACK, 0, 0);
        tcp_set_state(conn, TCP_FIN_WAIT_1);
        break;

    case TCP_ESTABLISHED:
        // TODO - queue FIN after sends
        tcp_tx(conn, conn->snd_nxt, TCP_FIN | TCP_ACK, 0, 0);
        tcp_set_state(conn, TCP_FIN_WAIT_1);
        break;

    case TCP_FIN_WAIT_1:
    case TCP_FIN_WAIT_2:
    case TCP_CLOSING:
    case TCP_LAST_ACK:
    case TCP_TIME_WAIT:
        if (conn->on_error)
        {
            conn->on_error(conn, TCP_CONN_CLOSING);
        }
        break;

    case TCP_CLOSE_WAIT:
        // TODO - queue FIN and state transition after sends
        tcp_tx(conn, conn->snd_nxt, TCP_FIN | TCP_ACK, 0, 0);
        tcp_set_state(conn, TCP_LAST_ACK);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void tcp_send(TCP_Conn* conn, const void* data, uint count)
{
    tcp_tx(conn, conn->snd_nxt, TCP_ACK, data, count);
}
