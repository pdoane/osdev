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

static u32 s_baseIsn;
static Link s_freeConns = { &s_freeConns, &s_freeConns };

Link g_tcpActiveConns = { &g_tcpActiveConns, &g_tcpActiveConns};

// ------------------------------------------------------------------------------------------------
// TCP state strings

const char *g_tcpStateStrs[] =
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
static bool TcpParseOptions(TcpOptions *opt, const u8 *p, const u8 *end)
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
            u8 optLen = *p++;

            if (optLen < 2)
            {
                return false;
            }

            const u8 *next = p + optLen - 2;
            if (next > end)
            {
                return false;
            }

            switch (type)
            {
            case OPT_MSS:
                opt->mss = NetSwap16(*(u16 *)p);
                break;
            }

            p = next;
        }
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
static void TcpPrint(const NetBuf *pkt)
{
    if (~g_netTrace & TRACE_TRANSPORT)
    {
        return;
    }

    if (pkt->start + sizeof(TcpHeader) > pkt->end)
    {
        return;
    }

    ChecksumHeader *phdr = (ChecksumHeader *)(pkt->start - sizeof(ChecksumHeader));
    char srcAddrStr[IPV4_ADDR_STRING_SIZE];
    char dstAddrStr[IPV4_ADDR_STRING_SIZE];
    Ipv4AddrToStr(srcAddrStr, sizeof(srcAddrStr), &phdr->src);
    Ipv4AddrToStr(dstAddrStr, sizeof(dstAddrStr), &phdr->dst);

    const TcpHeader *hdr = (const TcpHeader *)pkt->start;

    u16 srcPort = NetSwap16(hdr->srcPort);
    u16 dstPort = NetSwap16(hdr->dstPort);
    u32 seq = NetSwap32(hdr->seq);
    u32 ack = NetSwap32(hdr->ack);
    u16 windowSize = NetSwap16(hdr->windowSize);
    u16 checksum = NetSwap16(hdr->checksum);
    u16 urgent = NetSwap16(hdr->urgent);

    u16 checksum2 = NetChecksum(pkt->start - sizeof(ChecksumHeader), pkt->end);

    uint hdrLen = hdr->off >> 2;
    //const u8 *data = (pkt->start + hdrLen);
    uint dataLen = (pkt->end - pkt->start) - hdrLen;

    ConsolePrint("  TCP: src=%s:%d dst=%s:%d\n",
            srcAddrStr, srcPort, dstAddrStr, dstPort);
    ConsolePrint("  TCP: seq=%u ack=%u dataLen=%u\n",
            seq, ack, dataLen);
    ConsolePrint("  TCP: flags=%02x window=%u urgent=%u checksum=%u%c\n",
            hdr->flags, windowSize, urgent, checksum,
            checksum2 ? '!' : ' ');

    if (hdrLen > sizeof(TcpHeader))
    {
        const u8 *p = pkt->start + sizeof(TcpHeader);
        const u8 *end = p + hdrLen;

        TcpOptions opt;
        TcpParseOptions(&opt, p, end);

        if (opt.mss)
        {
            ConsolePrint("  TCP: mss=%u\n", opt.mss);
        }
    }
}

// ------------------------------------------------------------------------------------------------
static void TcpSetState(TcpConn *conn, uint state)
{
    uint oldState = conn->state;
    conn->state = state;

    if (conn->onState)
    {
        conn->onState(conn, oldState, state);
    }
}

// ------------------------------------------------------------------------------------------------
static TcpConn *TcpAlloc()
{
    Link *p = s_freeConns.next;
    if (p != &s_freeConns)
    {
        LinkRemove(p);
        return LinkData(p, TcpConn, link);
    }
    else
    {
        return VMAlloc(sizeof(TcpConn));
    }
}

// ------------------------------------------------------------------------------------------------
static void TcpFree(TcpConn *conn)
{
    if (conn->state != TCP_CLOSED)
    {
        TcpSetState(conn, TCP_CLOSED);
    }

    NetBuf *pkt;
    NetBuf *next;
    ListForEachSafe(pkt, next, conn->resequence, link)
    {
        LinkRemove(&pkt->link);
        NetReleaseBuf(pkt);
    }

    LinkMoveBefore(&s_freeConns, &conn->link);
}

// ------------------------------------------------------------------------------------------------
static void TcpSendPacket(TcpConn *conn, u32 seq, u8 flags, const void *data, uint count)
{
    NetBuf *pkt = NetAllocBuf();

    // Header
    TcpHeader *hdr = (TcpHeader *)pkt->start;
    hdr->srcPort = conn->localPort;
    hdr->dstPort = conn->remotePort;
    hdr->seq = seq;
    hdr->ack = flags & TCP_ACK ? conn->rcvNxt : 0;
    hdr->off = 0;
    hdr->flags = flags;
    hdr->windowSize = TCP_WINDOW_SIZE;
    hdr->checksum = 0;
    hdr->urgent = 0;
    TcpSwap(hdr);

    u8 *p = pkt->start + sizeof(TcpHeader);

    if (flags & TCP_SYN)
    {
        // Maximum Segment Size
        p[0] = OPT_MSS;
        p[1] = 4;
        *(u16 *)(p + 2) = NetSwap16(1460);
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
    ChecksumHeader *phdr = (ChecksumHeader *)(pkt->start - sizeof(ChecksumHeader));
    phdr->src = conn->localAddr;
    phdr->dst = conn->remoteAddr;
    phdr->reserved = 0;
    phdr->protocol = IP_PROTOCOL_TCP;
    phdr->len = NetSwap16(pkt->end - pkt->start);

    // Checksum
    u16 checksum = NetChecksum(pkt->start - sizeof(ChecksumHeader), pkt->end);
    hdr->checksum = NetSwap16(checksum);

    // Transmit
    TcpPrint(pkt);
    Ipv4SendIntf(conn->intf, &conn->nextAddr, &conn->remoteAddr, IP_PROTOCOL_TCP, pkt);

    // Update State
    conn->sndNxt += count;
    if (flags & (TCP_SYN | TCP_FIN))
    {
        ++conn->sndNxt;
    }
}

// ------------------------------------------------------------------------------------------------
static TcpConn *TcpFind(const Ipv4Addr *srcAddr, u16 srcPort,
    const Ipv4Addr *dstAddr, u16 dstPort)
{
    TcpConn *conn;
    ListForEach(conn, g_tcpActiveConns, link)
    {
        if (srcPort == conn->remotePort &&
            dstPort == conn->localPort &&
            Ipv4AddrEq(srcAddr, &conn->remoteAddr) &&
            Ipv4AddrEq(dstAddr, &conn->localAddr))
        {
            return conn;
        }
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
static void TcpError(TcpConn *conn, uint error)
{
    if (conn->onError)
    {
        conn->onError(conn, error);
    }

    TcpFree(conn);
}

// ------------------------------------------------------------------------------------------------
void TcpInit()
{
    // Compute base ISN from system clock and ticks since boot.  ISN is incremented every 4 us.
    DateTime dt;
    RtcGetTime(&dt);
    abs_time t = JoinTime(&dt);

    s_baseIsn = (t * 1000 - g_pitTicks) * 250;
}

// ------------------------------------------------------------------------------------------------
static void TcpRecvClosed(ChecksumHeader *phdr, TcpHeader *hdr)
{
    // Drop packet if this is a RST
    if (hdr->flags & TCP_RST)
    {
        return;
    }

    // Find an appropriate interface to route packet
    const Ipv4Addr *dstAddr = &phdr->src;
    const NetRoute *route = NetFindRoute(dstAddr);

    if (!route)
    {
        return;
    }

    // Create dummy connection for sending RST
    TcpConn rstConn;
    memset(&rstConn, 0, sizeof(TcpConn));

    rstConn.intf = route->intf;
    rstConn.localAddr = phdr->dst;
    rstConn.localPort = hdr->dstPort;
    rstConn.remoteAddr = phdr->src;
    rstConn.remotePort = hdr->srcPort;
    rstConn.nextAddr = *NetNextAddr(route, dstAddr);

    if (hdr->flags & TCP_ACK)
    {
        TcpSendPacket(&rstConn, hdr->ack, TCP_RST, 0, 0);
    }
    else
    {
        uint hdrLen = hdr->off >> 2;
        uint dataLen = phdr->len - hdrLen;

        rstConn.rcvNxt = hdr->seq + dataLen;

        TcpSendPacket(&rstConn, 0, TCP_RST | TCP_ACK, 0, 0);
    }
}

// ------------------------------------------------------------------------------------------------
static void TcpRecvSynSent(TcpConn *conn, TcpHeader *hdr)
{
    uint flags = hdr->flags;

    // Check for bad ACK first.
    if (flags & TCP_ACK)
    {
        if (SEQ_LE(hdr->ack, conn->iss) || SEQ_GT(hdr->ack, conn->sndNxt))
        {
            if (~flags & TCP_RST)
            {
                TcpSendPacket(conn, hdr->ack, TCP_RST, 0, 0);
            }

            return;
        }
    }

    // Check for RST
    if (flags & TCP_RST)
    {
        if (flags & TCP_ACK)
        {
            TcpError(conn, TCP_CONN_RESET);
        }

        return;
    }

    // Check SYN
    if (flags & TCP_SYN)
    {
        // SYN is set.  ACK is either ok or there was no ACK.  No RST.

        conn->irs = hdr->seq;
        conn->rcvNxt = hdr->seq + 1;

        if (flags & TCP_ACK)
        {
            conn->sndUna = hdr->ack;
            conn->sndWnd = hdr->windowSize;
            conn->sndWl1 = hdr->seq;
            conn->sndWl2 = hdr->ack;

            // TODO - Segments on the retransmission queue which are ack'd should be removed

            TcpSetState(conn, TCP_ESTABLISHED);
            TcpSendPacket(conn, conn->sndNxt, TCP_ACK, 0, 0);


            // TODO - Data queued for transmission may be included with the ACK.

            // TODO - If there is data in the segment, continue processing at the URG phase.
        }
        else
        {
            TcpSetState(conn, TCP_SYN_RECEIVED);

            // Resend ISS
            --conn->sndNxt;
            TcpSendPacket(conn, conn->sndNxt, TCP_SYN | TCP_ACK, 0, 0);
        }
    }
}

// ------------------------------------------------------------------------------------------------
static void TcpRecvRst(TcpConn *conn, TcpHeader *hdr)
{
    switch (conn->state)
    {
    case TCP_SYN_RECEIVED:
        // TODO - All segments on the retransmission queue should be removed

        // TODO - If initiated with a passive open, go to LISTEN state

        TcpError(conn, TCP_CONN_REFUSED);
        break;

    case TCP_ESTABLISHED:
    case TCP_FIN_WAIT_1:
    case TCP_FIN_WAIT_2:
    case TCP_CLOSE_WAIT:
        // TODO - All outstanding sends should receive "reset" responses

        TcpError(conn, TCP_CONN_RESET);
        break;

    case TCP_CLOSING:
    case TCP_LAST_ACK:
    case TCP_TIME_WAIT:
        TcpFree(conn);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
static void TcpRecvSyn(TcpConn *conn, TcpHeader *hdr)
{
    // TODO - All outstanding sends should receive "reset" responses

    TcpSendPacket(conn, 0, TCP_RST | TCP_ACK, 0, 0);

    TcpError(conn, TCP_CONN_RESET);
}

// ------------------------------------------------------------------------------------------------
static void TcpRecvAck(TcpConn *conn, TcpHeader *hdr)
{
    switch (conn->state)
    {
    case TCP_SYN_RECEIVED:
        if (conn->sndUna <= hdr->ack && hdr->ack <= conn->sndNxt)
        {
            conn->sndWnd = hdr->windowSize;
            conn->sndWl1 = hdr->seq;
            conn->sndWl2 = hdr->ack;
            TcpSetState(conn, TCP_ESTABLISHED);
        }
        else
        {
            TcpSendPacket(conn, hdr->ack, TCP_RST, 0, 0);
        }
        break;

    case TCP_ESTABLISHED:
    case TCP_FIN_WAIT_1:
    case TCP_FIN_WAIT_2:
    case TCP_CLOSE_WAIT:
    case TCP_CLOSING:
        // Handle expected acks
        if (SEQ_LE(conn->sndUna, hdr->ack) && SEQ_LE(hdr->ack, conn->sndNxt))
        {
            // Update acknowledged pointer
            conn->sndUna = hdr->ack;

            // Update send window
            if (SEQ_LT(conn->sndWl1, hdr->seq) ||
                (conn->sndWl1 == hdr->seq && SEQ_LE(conn->sndWl2, hdr->ack)))
            {
                conn->sndWnd = hdr->windowSize;
                conn->sndWl1 = hdr->seq;
                conn->sndWl2 = hdr->ack;
            }

            // TODO - remove segments on the retransmission queue which have been ack'd
            // TODO - acknowledge buffers which have sent to user
        }

        // Check for duplicate ack
        if (SEQ_LE(hdr->ack, conn->sndUna))
        {
            // TODO - anything to do here?
        }

        // Check for ack of unsent data
        if (SEQ_GT(hdr->ack, conn->sndNxt))
        {
            TcpSendPacket(conn, conn->sndNxt, TCP_ACK, 0, 0);
            return;
        }

        // Check for ack of FIN
        if (SEQ_GE(hdr->ack, conn->sndNxt))
        {
            // TODO - is this the right way to detect that our FIN has been ACK'd?
            if (conn->state == TCP_FIN_WAIT_1)
            {
                TcpSetState(conn, TCP_FIN_WAIT_2);
            }
            else if (conn->state == TCP_CLOSING)
            {
                TcpSetState(conn, TCP_TIME_WAIT);
                conn->mslWait = g_pitTicks + 2 * TCP_MSL;
            }
        }

        break;

    case TCP_LAST_ACK:
        // Check for ack of FIN
        if (SEQ_GE(hdr->ack, conn->sndNxt))
        {
            // TODO - is this the right way to detect that our FIN has been ACK'd?

            TcpFree(conn);
        }
        break;

    case TCP_TIME_WAIT:
        // This case is handled in the FIN processing step.
        break;
    }
}

// ------------------------------------------------------------------------------------------------
static void TcpRecvInsert(TcpConn *conn, NetBuf *pkt)
{
    NetBuf *prev;
    NetBuf *cur;
    NetBuf *next;

    uint dataLen = pkt->end - pkt->start;
    uint pktEnd = pkt->seq + dataLen;

    // Find location to insert packet
    ListForEach(cur, conn->resequence, link)
    {
        if (SEQ_LE(pkt->seq, cur->seq))
        {
            break;
        }
    }

    // Check if we already have some of this data in the previous packet.
    if (cur->link.prev != &conn->resequence)
    {
        prev = LinkData(cur->link.prev, NetBuf, link);
        uint prev_end = prev->seq + prev->end - prev->start;

        if (SEQ_GE(prev_end, pktEnd))
        {
            // Complete overlap with queued packet - drop incoming packet
            NetReleaseBuf(pkt);
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
            next = LinkData(cur->link.next, NetBuf, link);
            LinkRemove(&cur->link);
            NetReleaseBuf(cur);
            cur = next;
        }
    }

    // Trim/remove later packets that overlap
    while (&cur->link != &conn->resequence)
    {
        uint pktEnd = pkt->seq + dataLen;
        uint curEnd = cur->seq + cur->end - cur->start;

        if (SEQ_LT(pktEnd, cur->seq))
        {
            // No overlap
            break;
        }

        if (SEQ_LT(pktEnd, curEnd))
        {
            // Partial overlap - trim
            pkt->end -= pktEnd - cur->seq;
            break;
        }

        // Complete overlap - remove
        next = LinkData(cur->link.next, NetBuf, link);
        LinkRemove(&cur->link);
        NetReleaseBuf(cur);
        cur = next;
    }

    // Add packet to the queue
    LinkBefore(&cur->link, &pkt->link);
}

// ------------------------------------------------------------------------------------------------
static void TcpRecvProcess(TcpConn *conn)
{
    NetBuf *pkt;
    NetBuf *next;
    ListForEachSafe(pkt, next, conn->resequence, link)
    {
        if (conn->rcvNxt != pkt->seq)
        {
            break;
        }

        uint dataLen = pkt->end - pkt->start;
        conn->rcvNxt += dataLen;

        if (conn->onData)
        {
            conn->onData(conn, pkt->start, dataLen);
        }

        LinkRemove(&pkt->link);
        NetReleaseBuf(pkt);
    }
}

// ------------------------------------------------------------------------------------------------
static void TcpRecvData(TcpConn *conn, NetBuf *pkt)
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
        ++pkt->refCount;

        // Insert packet on to input queue sorted by sequence
        TcpRecvInsert(conn, pkt);

        // Process packets that are now in order
        TcpRecvProcess(conn);

        // Acknowledge receipt of data
        TcpSendPacket(conn, conn->sndNxt, TCP_ACK, 0, 0);
        break;

    default:
        // FIN has been received from the remote side - ignore the segment data.
        break;
    }
}

// ------------------------------------------------------------------------------------------------
static void TcpRecvFin(TcpConn *conn, TcpHeader *hdr)
{
    // TODO - signal the user "connection closing" and return any pending receives

    conn->rcvNxt = hdr->seq + 1;
    TcpSendPacket(conn, conn->sndNxt, TCP_ACK, 0, 0);

    switch (conn->state)
    {
    case TCP_SYN_RECEIVED:
    case TCP_ESTABLISHED:
        TcpSetState(conn, TCP_CLOSE_WAIT);
        break;

    case TCP_FIN_WAIT_1:
        if (SEQ_GE(hdr->ack, conn->sndNxt))
        {
            // TODO - is this the right way to detect that our FIN has been ACK'd?

            // TODO - turn off the other timers
            TcpSetState(conn, TCP_TIME_WAIT);
            conn->mslWait = g_pitTicks + 2 * TCP_MSL;
        }
        else
        {
            TcpSetState(conn, TCP_CLOSING);
        }
        break;

    case TCP_FIN_WAIT_2:
        // TODO - turn off the other timers
        TcpSetState(conn, TCP_TIME_WAIT);
        conn->mslWait = g_pitTicks + 2 * TCP_MSL;
        break;

    case TCP_CLOSE_WAIT:
    case TCP_CLOSING:
    case TCP_LAST_ACK:
        break;

    case TCP_TIME_WAIT:
        conn->mslWait = g_pitTicks + 2 * TCP_MSL;
        break;
    }
}

// ------------------------------------------------------------------------------------------------
static void TcpRecvGeneral(TcpConn *conn, TcpHeader *hdr, NetBuf *pkt)
{
    // Process segments not in the CLOSED, LISTEN, or SYN-SENT states.

    uint flags = hdr->flags;
    uint dataLen = pkt->end - pkt->start;

    // Check that sequence and segment data is acceptable
    if (!(SEQ_LE(conn->rcvNxt, hdr->seq) && SEQ_LE(hdr->seq + dataLen, conn->rcvNxt + conn->rcvWnd)))
    {
        // Unacceptable segment
        if (~flags & TCP_RST)
        {
            TcpSendPacket(conn, conn->sndNxt, TCP_ACK, 0, 0);
        }

        return;
    }

    // TODO - trim segment data?

    // Check RST bit
    if (flags & TCP_RST)
    {
        TcpRecvRst(conn, hdr);
        return;
    }

    // Check SYN bit
    if (flags & TCP_SYN)
    {
        TcpRecvSyn(conn, hdr);
    }

    // Check ACK
    if (~flags & TCP_ACK)
    {
        return;
    }

    TcpRecvAck(conn, hdr);

    // TODO - check URG

    // Process segment data
    if (dataLen)
    {
        TcpRecvData(conn, pkt);
    }

    // Check FIN - TODO, needs to handle out of sequence
    if (flags & TCP_FIN)
    {
        TcpRecvFin(conn, hdr);
    }
}

// ------------------------------------------------------------------------------------------------
void TcpRecv(NetIntf *intf, const Ipv4Header *ipHdr, NetBuf *pkt)
{
    // Validate packet header
    if (pkt->start + sizeof(TcpHeader) > pkt->end)
    {
        return;
    }

    // Assemble Pseudo Header
    Ipv4Addr srcAddr = ipHdr->src;
    Ipv4Addr dstAddr = ipHdr->dst;
    u8 protocol = ipHdr->protocol;

    ChecksumHeader *phdr = (ChecksumHeader *)(pkt->start - sizeof(ChecksumHeader));
    phdr->src = srcAddr;
    phdr->dst = dstAddr;
    phdr->reserved = 0;
    phdr->protocol = protocol;
    phdr->len = NetSwap16(pkt->end - pkt->start);

    TcpPrint(pkt);

    // Validate checksum
    if (NetChecksum(pkt->start - sizeof(ChecksumHeader), pkt->end))
    {
        return;
    }

    // Process packet
    TcpHeader *hdr = (TcpHeader *)pkt->start;
    TcpSwap(hdr);
    phdr->len = NetSwap16(phdr->len);

    // Find connection associated with packet
    TcpConn *conn = TcpFind(&phdr->src, hdr->srcPort, &phdr->dst, hdr->dstPort);
    if (!conn || conn->state == TCP_CLOSED)
    {
        TcpRecvClosed(phdr, hdr);
        return;
    }

    // Process packet by state
    if (conn->state == TCP_LISTEN)
    {
    }
    else if (conn->state == TCP_SYN_SENT)
    {
        TcpRecvSynSent(conn, hdr);
    }
    else
    {
        // Update packet to point to data, and store parts of
        // header needed for out of order handling.
        uint hdrLen = hdr->off >> 2;
        pkt->start += hdrLen;
        pkt->seq = hdr->seq;
        pkt->flags = hdr->flags;

        TcpRecvGeneral(conn, hdr, pkt);
    }
}

// ------------------------------------------------------------------------------------------------
void TcpPoll()
{
    TcpConn *conn;
    TcpConn *next;
    ListForEachSafe(conn, next, g_tcpActiveConns, link)
    {
        if (conn->state == TCP_TIME_WAIT && SEQ_GE(g_pitTicks, conn->mslWait))
        {
            TcpFree(conn);
        }
    }
}

// ------------------------------------------------------------------------------------------------
void TcpSwap(TcpHeader *hdr)
{
    hdr->srcPort = NetSwap16(hdr->srcPort);
    hdr->dstPort = NetSwap16(hdr->dstPort);
    hdr->seq = NetSwap32(hdr->seq);
    hdr->ack = NetSwap32(hdr->ack);
    hdr->windowSize = NetSwap16(hdr->windowSize);
    hdr->checksum = NetSwap16(hdr->checksum);
    hdr->urgent = NetSwap16(hdr->urgent);
}

// ------------------------------------------------------------------------------------------------
TcpConn *TcpCreate()
{
    TcpConn *conn = TcpAlloc();
    memset(conn, 0, sizeof(TcpConn));
    conn->resequence.next = &conn->resequence;
    conn->resequence.prev = &conn->resequence;

    return conn;
}

// ------------------------------------------------------------------------------------------------
bool TcpConnect(TcpConn *conn, const Ipv4Addr *addr, u16 port)
{
    // Find network interface through the routing table.
    const NetRoute *route = NetFindRoute(addr);
    if (!route)
    {
        return false;
    }

    NetIntf *intf = route->intf;

    // Initialize connection
    conn->intf = intf;
    conn->localAddr = intf->ipAddr;
    conn->nextAddr = *NetNextAddr(route, addr);
    conn->remoteAddr = *addr;
    conn->localPort = NetEphemeralPort();
    conn->remotePort = port;

    u32 isn = s_baseIsn + g_pitTicks * 250;

    conn->sndUna = isn;
    conn->sndNxt = isn;
    conn->sndWnd = TCP_WINDOW_SIZE;
    conn->sndUP = 0;
    conn->sndWl1 = 0;
    conn->sndWl2 = 0;
    conn->iss = isn;

    conn->rcvNxt = 0;
    conn->rcvWnd = TCP_WINDOW_SIZE;
    conn->rcvUP = 0;
    conn->irs = 0;

    // Link to active connections
    LinkBefore(&g_tcpActiveConns, &conn->link);

    // Issue SYN segment
    TcpSendPacket(conn, conn->sndNxt, TCP_SYN, 0, 0);
    TcpSetState(conn, TCP_SYN_SENT);

    return true;
}

// ------------------------------------------------------------------------------------------------
void TcpClose(TcpConn *conn)
{
    switch (conn->state)
    {
    case TCP_CLOSED:
        TcpFree(conn);
        break;

    case TCP_LISTEN:
        TcpFree(conn);
        break;

    case TCP_SYN_SENT:
        // TODO - cancel queued sends
        TcpFree(conn);
        break;

    case TCP_SYN_RECEIVED:
        // TODO - if sends have been issued or queued, wait for ESTABLISHED
        // before entering FIN-WAIT-1
        TcpSendPacket(conn, conn->sndNxt, TCP_FIN | TCP_ACK, 0, 0);
        TcpSetState(conn, TCP_FIN_WAIT_1);
        break;

    case TCP_ESTABLISHED:
        // TODO - queue FIN after sends
        TcpSendPacket(conn, conn->sndNxt, TCP_FIN | TCP_ACK, 0, 0);
        TcpSetState(conn, TCP_FIN_WAIT_1);
        break;

    case TCP_FIN_WAIT_1:
    case TCP_FIN_WAIT_2:
    case TCP_CLOSING:
    case TCP_LAST_ACK:
    case TCP_TIME_WAIT:
        if (conn->onError)
        {
            conn->onError(conn, TCP_CONN_CLOSING);
        }
        break;

    case TCP_CLOSE_WAIT:
        // TODO - queue FIN and state transition after sends
        TcpSendPacket(conn, conn->sndNxt, TCP_FIN | TCP_ACK, 0, 0);
        TcpSetState(conn, TCP_LAST_ACK);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void TcpSend(TcpConn *conn, const void *data, uint count)
{
    TcpSendPacket(conn, conn->sndNxt, TCP_ACK, data, count);
}
