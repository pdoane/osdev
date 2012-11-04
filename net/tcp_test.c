// ------------------------------------------------------------------------------------------------
// net/tcp_test.c
// ------------------------------------------------------------------------------------------------

#include "test/test.h"
#include "net/checksum.h"
#include "net/ipv4.h"
#include "net/loopback.h"
#include "net/route.h"
#include "net/swap.h"
#include "net/tcp.h"
#include "stdlib/string.h"
#include "time/time.h"

#include <stdarg.h>
#include <stdio.h>

static NetIntf *s_intf;
static Ipv4Addr s_ipAddr = { { { 127, 0, 0, 1 } } };
static Ipv4Addr s_subnetMask = { { { 255, 255, 255, 255 } } };

// ------------------------------------------------------------------------------------------------
// Packets

typedef struct Packet
{
    Link link;
    ChecksumHeader phdr;
    u8 data[1500];
    u8 *end;
} Packet;

// ------------------------------------------------------------------------------------------------
// Mocked dependencies

u8 g_netTrace;
u32 g_pitTicks;
static Link s_outPackets = { &s_outPackets, &s_outPackets };

void RtcGetTime(DateTime *dt)
{
    SplitTime(dt, 0, 0);
}

void ConsolePrint(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void Ipv4SendIntf(NetIntf *intf, const Ipv4Addr *nextAddr,
    const Ipv4Addr *dstAddr, u8 protocol, NetBuf *pkt)
{
    uint len = pkt->end - pkt->start;

    Packet *packet = malloc(sizeof(Packet));
    packet->phdr.src = intf->ipAddr;
    packet->phdr.dst = *dstAddr;
    packet->phdr.reserved = 0;
    packet->phdr.protocol = protocol;
    packet->phdr.len = NetSwap16(len);

    memcpy(packet->data, pkt->start, len);
    packet->end = packet->data + len;

    LinkBefore(&s_outPackets, &packet->link);

    NetReleaseBuf(pkt);
}

void *VMAlloc(uint size)
{
    return malloc(size);
}

// ------------------------------------------------------------------------------------------------
static uint outError;

static void OnError(TcpConn *conn, uint error)
{
    ASSERT_EQ_UINT(outError, 0);
    outError = error;
}

// ------------------------------------------------------------------------------------------------
static TcpConn *CreateConn()
{
    TcpConn *conn = TcpCreate();
    conn->onError = OnError;
    outError = 0;
    return conn;
}

// ------------------------------------------------------------------------------------------------
static void TcpInput(NetBuf *pkt)
{
    TcpHeader *tcpHdr = (TcpHeader *)pkt->start;
    TcpSwap(tcpHdr);

    // Data
    pkt->end = pkt->start + sizeof(TcpHeader);

    // Pseudo Header
    ChecksumHeader *phdr = (ChecksumHeader *)(pkt->start - sizeof(ChecksumHeader));
    phdr->src = s_ipAddr;
    phdr->dst = s_ipAddr;
    phdr->reserved = 0;
    phdr->protocol = IP_PROTOCOL_TCP;
    phdr->len = NetSwap16(pkt->end - pkt->start);

    // Checksum
    u16 checksum = NetChecksum(pkt->start - sizeof(ChecksumHeader), pkt->end);
    tcpHdr->checksum = NetSwap16(checksum);

    // IP Header
    Ipv4Header *ipHdr = (Ipv4Header *)(pkt->start - sizeof(Ipv4Header));
    ipHdr->verIhl = (4 << 4) | 5;
    ipHdr->tos = 0;
    ipHdr->len = NetSwap16(pkt->end - pkt->start);
    ipHdr->id = NetSwap16(0);
    ipHdr->offset = NetSwap16(0);
    ipHdr->ttl = 64;
    ipHdr->protocol = IP_PROTOCOL_TCP;
    ipHdr->checksum = 0;
    ipHdr->src = s_ipAddr;
    ipHdr->dst = s_ipAddr;

    // Receive
    TcpRecv(s_intf, ipHdr, pkt);

    NetReleaseBuf(pkt);
}

// ------------------------------------------------------------------------------------------------
static void ValidateChecksum(Packet *pkt)
{
    u8 *phdrData = (u8 *)&pkt->phdr;
    u8 *phdrEnd = phdrData + sizeof(ChecksumHeader);

    uint sum = 0;
    sum = NetChecksumAcc(phdrData, phdrEnd, sum);
    sum = NetChecksumAcc(pkt->data, pkt->end, sum);
    u16 checksum = NetChecksumFinal(sum);

    ASSERT_EQ_UINT(checksum, 0);
}

// ------------------------------------------------------------------------------------------------
static Packet *PopPacket()
{
    ASSERT_TRUE(!ListIsEmpty(&s_outPackets));

    Packet *packet = LinkData(s_outPackets.next, Packet, link);
    LinkRemove(&packet->link);

    ValidateChecksum(packet);
    return packet;
}

// ------------------------------------------------------------------------------------------------
static void ExpectError(uint error)
{
    ASSERT_EQ_UINT(outError, error);
    outError = 0;
}

// ------------------------------------------------------------------------------------------------
static TcpHeader *PrepareInPkt(TcpConn *conn, NetBuf *inPkt, uint seq, uint ack, uint flags)
{
    TcpHeader *hdr = (TcpHeader *)inPkt->start;
    hdr->srcPort = conn->remotePort;
    hdr->dstPort = conn->localPort;
    hdr->seq = seq;
    hdr->ack = ack;
    hdr->off = 5 << 4;
    hdr->flags = flags;
    hdr->windowSize = TCP_WINDOW_SIZE;
    hdr->checksum = 0;
    hdr->urgent = 0;

    return hdr;
}

// ------------------------------------------------------------------------------------------------
static void TestCaseBegin(uint state, const char *cond, const char *action)
{
    printf("-- %12s: %-20s - %s\n", g_tcpStateStrs[state], cond, action);
}

// ------------------------------------------------------------------------------------------------
static void TestCaseEnd()
{
    ASSERT_TRUE(ListIsEmpty(&s_outPackets));
    ASSERT_TRUE(ListIsEmpty(&g_tcpActiveConns));
    ASSERT_EQ_INT(g_netBufAllocCount, 0);
    ASSERT_EQ_UINT(outError, 0);
}

// ------------------------------------------------------------------------------------------------
static void TestSetup()
{
    // Create net interface
    s_intf = NetIntfCreate();
    s_intf->ethAddr = g_nullEthAddr;
    s_intf->ipAddr = s_ipAddr;
    s_intf->name = "test";
    s_intf->poll = 0;
    s_intf->send = 0;
    s_intf->devSend = 0;

    //NetIntfAdd(s_intf);

    // Add routing entry
    NetAddRoute(&s_ipAddr, &s_subnetMask, 0, s_intf);
}

// ------------------------------------------------------------------------------------------------
static void EnterState(TcpConn *conn, uint state)
{
    NetBuf *inPkt;
    TcpHeader *inHdr;
    Packet *outPkt;
    //TcpHeader *outHdr;

    switch (state)
    {
    case TCP_SYN_SENT:
        ASSERT_TRUE(TcpConnect(conn, &s_ipAddr, 80));

        outPkt = PopPacket();
        free(outPkt);
        break;

    case TCP_SYN_RECEIVED:
        EnterState(conn, TCP_SYN_SENT);

        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, 1000, 0, TCP_SYN);
        TcpInput(inPkt);

        outPkt = PopPacket();
        free(outPkt);
        break;

    case TCP_ESTABLISHED:
        EnterState(conn, TCP_SYN_SENT);

        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt, TCP_SYN | TCP_ACK);
        TcpInput(inPkt);

        outPkt = PopPacket();
        free(outPkt);
        break;

    case TCP_FIN_WAIT_1:
        EnterState(conn, TCP_ESTABLISHED);

        TcpClose(conn);

        outPkt = PopPacket();
        free(outPkt);
        break;

    case TCP_FIN_WAIT_2:
        EnterState(conn, TCP_FIN_WAIT_1);

        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt, TCP_ACK);
        TcpInput(inPkt);
        break;

    case TCP_CLOSE_WAIT:
        EnterState(conn, TCP_ESTABLISHED);

        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt, TCP_FIN | TCP_ACK);
        TcpInput(inPkt);

        outPkt = PopPacket();
        free(outPkt);
        break;

    case TCP_CLOSING:
        EnterState(conn, TCP_FIN_WAIT_1);

        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt - 1, TCP_FIN | TCP_ACK);
        TcpInput(inPkt);

        outPkt = PopPacket();
        free(outPkt);
        break;

    case TCP_LAST_ACK:
        EnterState(conn, TCP_CLOSE_WAIT);

        TcpClose(conn);

        outPkt = PopPacket();
        free(outPkt);
        break;

    case TCP_TIME_WAIT:
        EnterState(conn, TCP_FIN_WAIT_1);

        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt, TCP_FIN | TCP_ACK);
        TcpInput(inPkt);

        outPkt = PopPacket();
        free(outPkt);
        break;

    default:
        ASSERT_EQ_UINT(state, 0);
        break;
    }

    ASSERT_EQ_UINT(conn->state, state);
    ASSERT_TRUE(ListIsEmpty(&s_outPackets));
}

// ------------------------------------------------------------------------------------------------
static void ExitState(TcpConn *conn, uint state)
{
    NetBuf *inPkt;
    TcpHeader *inHdr;
    Packet *outPkt;
    TcpHeader *outHdr;

    ASSERT_EQ_UINT(conn->state, state);
    ASSERT_TRUE(ListIsEmpty(&s_outPackets));

    switch (state)
    {
    case TCP_CLOSED:
        TcpClose(conn);
        break;

    case TCP_SYN_SENT:
        TcpClose(conn);
        break;

    case TCP_SYN_RECEIVED:
        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt, TCP_ACK);
        TcpInput(inPkt);

        ExitState(conn, TCP_ESTABLISHED);
        break;

    case TCP_ESTABLISHED:
        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt, TCP_FIN | TCP_ACK);
        TcpInput(inPkt);

        outPkt = PopPacket();
        outHdr = (TcpHeader *)outPkt->data;
        TcpSwap(outHdr);
        ASSERT_EQ_UINT(outHdr->srcPort, conn->localPort);
        ASSERT_EQ_UINT(outHdr->dstPort, conn->remotePort);
        ASSERT_EQ_UINT(outHdr->seq, conn->sndNxt);
        ASSERT_EQ_UINT(outHdr->ack, conn->rcvNxt);
        ASSERT_EQ_HEX8(outHdr->flags, TCP_ACK);
        free(outPkt);

        ExitState(conn, TCP_CLOSE_WAIT);
        break;

    case TCP_FIN_WAIT_1:
        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt, TCP_FIN | TCP_ACK);
        TcpInput(inPkt);

        outPkt = PopPacket();
        outHdr = (TcpHeader *)outPkt->data;
        TcpSwap(outHdr);
        ASSERT_EQ_UINT(outHdr->srcPort, conn->localPort);
        ASSERT_EQ_UINT(outHdr->dstPort, conn->remotePort);
        ASSERT_EQ_UINT(outHdr->seq, conn->sndNxt);
        ASSERT_EQ_UINT(outHdr->ack, conn->rcvNxt);
        ASSERT_EQ_HEX8(outHdr->flags, TCP_ACK);
        free(outPkt);

        ExitState(conn, TCP_TIME_WAIT);
        break;

    case TCP_FIN_WAIT_2:
        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt, TCP_FIN | TCP_ACK);
        TcpInput(inPkt);

        outPkt = PopPacket();
        outHdr = (TcpHeader *)outPkt->data;
        TcpSwap(outHdr);
        ASSERT_EQ_UINT(outHdr->srcPort, conn->localPort);
        ASSERT_EQ_UINT(outHdr->dstPort, conn->remotePort);
        ASSERT_EQ_UINT(outHdr->seq, conn->sndNxt);
        ASSERT_EQ_UINT(outHdr->ack, conn->rcvNxt);
        ASSERT_EQ_HEX8(outHdr->flags, TCP_ACK);
        free(outPkt);

        ExitState(conn, TCP_TIME_WAIT);
        break;

    case TCP_CLOSE_WAIT:
        TcpClose(conn);

        outPkt = PopPacket();
        outHdr = (TcpHeader *)outPkt->data;
        TcpSwap(outHdr);
        ASSERT_EQ_UINT(outHdr->srcPort, conn->localPort);
        ASSERT_EQ_UINT(outHdr->dstPort, conn->remotePort);
        ASSERT_EQ_UINT(outHdr->seq, conn->sndNxt - 1);
        ASSERT_EQ_UINT(outHdr->ack, conn->rcvNxt);
        ASSERT_EQ_HEX8(outHdr->flags, TCP_FIN | TCP_ACK);
        free(outPkt);

        ExitState(conn, TCP_LAST_ACK);
        break;

    case TCP_CLOSING:
        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt, TCP_ACK);
        TcpInput(inPkt);

        ExitState(conn, TCP_TIME_WAIT);
        break;

    case TCP_LAST_ACK:
        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt, TCP_ACK);
        TcpInput(inPkt);
        break;

    case TCP_TIME_WAIT:
        g_pitTicks += 2 * TCP_MSL;
        TcpPoll();
        break;

    default:
        ASSERT_EQ_UINT(state, 0);
        break;
    }

    ASSERT_EQ_UINT(conn->state, TCP_CLOSED);
}

// ------------------------------------------------------------------------------------------------
int main(int argc, const char **argv)
{
    // Common variables
    NetBuf *inPkt;
    TcpHeader *inHdr;
    Packet *outPkt;
    TcpHeader *outHdr;
    TcpConn *conn;

    TestSetup();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_CLOSED, "RST", "segment dropped");

    inPkt = NetAllocBuf();
    inHdr = (TcpHeader *)inPkt->start;
    inHdr->srcPort = 100;
    inHdr->dstPort = 101;
    inHdr->seq = 1;
    inHdr->ack = 2;
    inHdr->off = 5 << 4;
    inHdr->flags = TCP_RST;
    inHdr->windowSize = TCP_WINDOW_SIZE;
    inHdr->checksum = 0;
    inHdr->urgent = 0;
    TcpInput(inPkt);

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_CLOSED, "ACK", "RST sent");

    inPkt = NetAllocBuf();
    inHdr = (TcpHeader *)inPkt->start;
    inHdr->srcPort = 100;
    inHdr->dstPort = 101;
    inHdr->seq = 1;
    inHdr->ack = 2;
    inHdr->off = 5 << 4;
    inHdr->flags = TCP_ACK;
    inHdr->windowSize = TCP_WINDOW_SIZE;
    inHdr->checksum = 0;
    inHdr->urgent = 0;
    TcpInput(inPkt);

    outPkt = PopPacket();
    outHdr = (TcpHeader *)outPkt->data;
    TcpSwap(outHdr);
    ASSERT_EQ_UINT(outHdr->srcPort, 101);
    ASSERT_EQ_UINT(outHdr->dstPort, 100);
    ASSERT_EQ_UINT(outHdr->seq, 2);
    ASSERT_EQ_UINT(outHdr->ack, 0);
    ASSERT_EQ_HEX8(outHdr->flags, TCP_RST);
    free(outPkt);

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_CLOSED, "no ACK", "RST/ACK sent");

    inPkt = NetAllocBuf();
    inHdr = (TcpHeader *)inPkt->start;
    inHdr->srcPort = 100;
    inHdr->dstPort = 101;
    inHdr->seq = 1;
    inHdr->ack = 2;
    inHdr->off = 5 << 4;
    inHdr->flags = 0;
    inHdr->windowSize = TCP_WINDOW_SIZE;
    inHdr->checksum = 0;
    inHdr->urgent = 0;
    TcpInput(inPkt);

    outPkt = PopPacket();
    outHdr = (TcpHeader *)outPkt->data;
    TcpSwap(outHdr);
    ASSERT_EQ_UINT(outHdr->srcPort, 101);
    ASSERT_EQ_UINT(outHdr->dstPort, 100);
    ASSERT_EQ_UINT(outHdr->seq, 0);
    ASSERT_EQ_UINT(outHdr->ack, 1);
    ASSERT_EQ_HEX8(outHdr->flags, TCP_RST | TCP_ACK);
    free(outPkt);

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_CLOSED, "connect", "goto SYN_SENT");

    conn = CreateConn();

    ASSERT_TRUE(TcpConnect(conn, &s_ipAddr, 80));

    outPkt = PopPacket();
    outHdr = (TcpHeader *)outPkt->data;
    TcpSwap(outHdr);
    ASSERT_TRUE(outHdr->srcPort >= 49152);
    ASSERT_EQ_UINT(outHdr->dstPort, 80);
    ASSERT_EQ_UINT(outHdr->seq, conn->iss);
    ASSERT_EQ_UINT(outHdr->ack, 0);
    ASSERT_EQ_HEX8(outHdr->flags, TCP_SYN);
    ASSERT_EQ_UINT(outHdr->windowSize, TCP_WINDOW_SIZE);
    ASSERT_EQ_UINT(outHdr->urgent, 0);
    free(outPkt);

    ExitState(conn, TCP_SYN_SENT);

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_SYN_SENT, "Bad ACK, no RST", "RST sent");

    conn = CreateConn();
    EnterState(conn, TCP_SYN_SENT);

    inPkt = NetAllocBuf();
    inHdr = PrepareInPkt(conn, inPkt, 1000, conn->iss, TCP_ACK);
    TcpInput(inPkt);

    outPkt = PopPacket();
    outHdr = (TcpHeader *)outPkt->data;
    TcpSwap(outHdr);
    ASSERT_EQ_UINT(outHdr->srcPort, conn->localPort);
    ASSERT_EQ_UINT(outHdr->dstPort, conn->remotePort);
    ASSERT_EQ_UINT(outHdr->seq, inHdr->ack);
    ASSERT_EQ_UINT(outHdr->ack, 0);
    ASSERT_EQ_HEX8(outHdr->flags, TCP_RST);
    free(outPkt);

    ExitState(conn, TCP_SYN_SENT);

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_SYN_SENT, "Bad ACK, RST", "segment dropped");

    conn = CreateConn();
    EnterState(conn, TCP_SYN_SENT);

    inPkt = NetAllocBuf();
    inHdr = PrepareInPkt(conn, inPkt, 1000, conn->iss, TCP_RST | TCP_ACK);
    TcpInput(inPkt);

    ExitState(conn, TCP_SYN_SENT);

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_SYN_SENT, "ACK, RST", "conn locally reset");

    conn = CreateConn();
    EnterState(conn, TCP_SYN_SENT);

    inPkt = NetAllocBuf();
    inHdr = PrepareInPkt(conn, inPkt, 1000, conn->iss + 1, TCP_RST | TCP_ACK);
    TcpInput(inPkt);

    ExpectError(TCP_CONN_RESET);
    ExitState(conn, TCP_CLOSED);

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_SYN_SENT, "no ACK, RST", "segment dropped");

    conn = CreateConn();
    EnterState(conn, TCP_SYN_SENT);

    inPkt = NetAllocBuf();
    inHdr = PrepareInPkt(conn, inPkt, 1000, conn->iss + 1, TCP_RST);
    TcpInput(inPkt);

    ExitState(conn, TCP_SYN_SENT);

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_SYN_SENT, "SYN, ACK", "goto ESTABLISHED");

    conn = CreateConn();
    EnterState(conn, TCP_SYN_SENT);

    inPkt = NetAllocBuf();
    inHdr = PrepareInPkt(conn, inPkt, 1000, conn->iss + 1, TCP_SYN | TCP_ACK);
    TcpInput(inPkt);

    ASSERT_EQ_UINT(conn->irs, 1000);
    ASSERT_EQ_UINT(conn->rcvNxt, 1001);

    outPkt = PopPacket();
    outHdr = (TcpHeader *)outPkt->data;
    TcpSwap(outHdr);
    ASSERT_EQ_UINT(outHdr->srcPort, conn->localPort);
    ASSERT_EQ_UINT(outHdr->dstPort, conn->remotePort);
    ASSERT_EQ_UINT(outHdr->seq, conn->iss + 1);
    ASSERT_EQ_UINT(outHdr->ack, 1001);
    ASSERT_EQ_HEX8(outHdr->flags, TCP_ACK);
    free(outPkt);

    ExitState(conn, TCP_ESTABLISHED);

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_SYN_SENT, "SYN, no ACK", "goto SYN_RECEIVED, resend SYN,ACK");

    conn = CreateConn();
    EnterState(conn, TCP_SYN_SENT);

    inPkt = NetAllocBuf();
    inHdr = PrepareInPkt(conn, inPkt, 1000, 0, TCP_SYN);
    TcpInput(inPkt);

    ASSERT_EQ_UINT(conn->irs, 1000);
    ASSERT_EQ_UINT(conn->rcvNxt, 1001);

    outPkt = PopPacket();
    outHdr = (TcpHeader *)outPkt->data;
    TcpSwap(outHdr);
    ASSERT_EQ_UINT(outHdr->srcPort, conn->localPort);
    ASSERT_EQ_UINT(outHdr->dstPort, conn->remotePort);
    ASSERT_EQ_UINT(outHdr->seq, conn->iss);
    ASSERT_EQ_UINT(outHdr->ack, 1001);
    ASSERT_EQ_HEX8(outHdr->flags, TCP_SYN | TCP_ACK);
    free(outPkt);

    ExitState(conn, TCP_SYN_RECEIVED);

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    uint generalStates[] =
    {
        TCP_SYN_RECEIVED,
        TCP_ESTABLISHED,
        TCP_FIN_WAIT_1,
        TCP_FIN_WAIT_2,
        TCP_CLOSE_WAIT,
        TCP_CLOSING,
        TCP_LAST_ACK,
        TCP_TIME_WAIT,
        0,
    };

    for (uint *pState = generalStates; *pState; ++pState)
    {
        uint state = *pState;

        TestCaseBegin(state, "Bad seq, no RST", "resend ACK");

        conn = CreateConn();
        EnterState(conn, state);

        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt - 1, conn->sndNxt, TCP_ACK);
        TcpInput(inPkt);

        outPkt = PopPacket();
        outHdr = (TcpHeader *)outPkt->data;
        TcpSwap(outHdr);
        ASSERT_EQ_UINT(outHdr->srcPort, conn->localPort);
        ASSERT_EQ_UINT(outHdr->dstPort, conn->remotePort);
        ASSERT_EQ_UINT(outHdr->seq, conn->sndNxt);
        ASSERT_EQ_UINT(outHdr->ack, conn->rcvNxt);
        ASSERT_EQ_HEX8(outHdr->flags, TCP_ACK);
        free(outPkt);

        ExitState(conn, state);

        TestCaseEnd();
    }

    // --------------------------------------------------------------------------------------------
    for (uint *pState = generalStates; *pState; ++pState)
    {
        uint state = *pState;

        TestCaseBegin(state, "Bad seq, RST", "segment dropped");

        conn = CreateConn();
        EnterState(conn, state);

        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt - 1, conn->sndNxt, TCP_RST | TCP_ACK);
        TcpInput(inPkt);

        ExitState(conn, state);

        TestCaseEnd();
    }

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_SYN_RECEIVED, "RST, active", "conn refused");

    conn = CreateConn();
    EnterState(conn, TCP_SYN_RECEIVED);

    inPkt = NetAllocBuf();
    inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, 0, TCP_RST);
    TcpInput(inPkt);

    ExpectError(TCP_CONN_REFUSED);

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    uint rstStates1[] =
    {
        TCP_ESTABLISHED,
        TCP_FIN_WAIT_1,
        TCP_FIN_WAIT_2,
        TCP_CLOSE_WAIT,
        0,
    };

    for (uint *pState = rstStates1; *pState; ++pState)
    {
        uint state = *pState;

        TestCaseBegin(state, "RST", "conn reset");

        conn = CreateConn();
        EnterState(conn, state);

        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, 0, TCP_RST);
        TcpInput(inPkt);

        ExpectError(TCP_CONN_RESET);

        TestCaseEnd();
    }

    // --------------------------------------------------------------------------------------------
    uint rstStates2[] =
    {
        TCP_CLOSING,
        TCP_LAST_ACK,
        TCP_TIME_WAIT,
        0,
    };

    for (uint *pState = rstStates2; *pState; ++pState)
    {
        uint state = *pState;

        TestCaseBegin(state, "RST", "conn closed");

        conn = CreateConn();
        EnterState(conn, state);

        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, 0, TCP_RST);
        TcpInput(inPkt);

        TestCaseEnd();
    }

    // --------------------------------------------------------------------------------------------
    for (uint *pState = generalStates; *pState; ++pState)
    {
        uint state = *pState;

        TestCaseBegin(state, "SYN", "conn reset, RST sent");

        conn = CreateConn();
        EnterState(conn, state);

        u16 localPort = conn->localPort;
        u16 remotePort = conn->remotePort;
        u32 rcvNxt = conn->rcvNxt;

        inPkt = NetAllocBuf();
        inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt, TCP_SYN);
        TcpInput(inPkt);

        outPkt = PopPacket();
        outHdr = (TcpHeader *)outPkt->data;
        TcpSwap(outHdr);
        ASSERT_EQ_UINT(outHdr->srcPort, localPort);
        ASSERT_EQ_UINT(outHdr->dstPort, remotePort);
        ASSERT_EQ_UINT(outHdr->seq, 0);
        ASSERT_EQ_UINT(outHdr->ack, rcvNxt);
        ASSERT_EQ_HEX8(outHdr->flags, TCP_RST | TCP_ACK);
        free(outPkt);

        ExpectError(TCP_CONN_RESET);

        TestCaseEnd();
    }

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_SYN_RECEIVED, "bad ACK", "RST sent");

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_SYN_RECEIVED, "ACK", "goto ESTABLISHED");

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    uint ackStates[] =
    {
        TCP_ESTABLISHED,
        TCP_FIN_WAIT_1,
        TCP_FIN_WAIT_2,
        TCP_CLOSE_WAIT,
        TCP_CLOSING,
        0
    };

    for (uint *pState = ackStates; *pState; ++pState)
    {
        uint state = *pState;
        TestCaseBegin(state, "ACK", "update pointers");

        TestCaseEnd();
    }

    // --------------------------------------------------------------------------------------------
    for (uint *pState = ackStates; *pState; ++pState)
    {
        uint state = *pState;
        TestCaseBegin(state, "dup ACK", "ignore");

        TestCaseEnd();
    }

    // --------------------------------------------------------------------------------------------
    for (uint *pState = ackStates; *pState; ++pState)
    {
        uint state = *pState;
        TestCaseBegin(state, "unsent ACK", "resend ACK");

        TestCaseEnd();
    }

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_FIN_WAIT_1, "ACK, FIN not ACK'd", "ignore");

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_CLOSING, "ACK, FIN not ACK'd", "ignore");

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_FIN_WAIT_1, "ACK, FIN ACK'd", "goto FIN-WAIT-2");

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_CLOSING, "ACK, FIN ACK'd", "goto TIME-WAIT");

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_LAST_ACK, "ACK, FIN not ACK'd", "ignore");

    conn = CreateConn();
    EnterState(conn, TCP_LAST_ACK);

    inPkt = NetAllocBuf();
    inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt - 1, TCP_ACK);
    TcpInput(inPkt);

    ExitState(conn, TCP_LAST_ACK);

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_LAST_ACK, "ACK, FIN ACK'd", "goto CLOSED");

    conn = CreateConn();
    EnterState(conn, TCP_LAST_ACK);

    inPkt = NetAllocBuf();
    inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt, TCP_ACK);
    TcpInput(inPkt);

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_TIME_WAIT, "ACK, no FIN", "ignore");

    conn = CreateConn();
    EnterState(conn, TCP_TIME_WAIT);

    inPkt = NetAllocBuf();
    inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt, TCP_ACK);
    TcpInput(inPkt);

    ExitState(conn, TCP_TIME_WAIT);

    TestCaseEnd();

    // --------------------------------------------------------------------------------------------
    TestCaseBegin(TCP_TIME_WAIT, "FIN", "reset 2MSL timer");

    conn = CreateConn();
    EnterState(conn, TCP_TIME_WAIT);

    g_pitTicks += 1000;
    inPkt = NetAllocBuf();
    inHdr = PrepareInPkt(conn, inPkt, conn->rcvNxt, conn->sndNxt, TCP_FIN | TCP_ACK);
    TcpInput(inPkt);

    outPkt = PopPacket();
    outHdr = (TcpHeader *)outPkt->data;
    TcpSwap(outHdr);
    ASSERT_EQ_UINT(outHdr->srcPort, conn->localPort);
    ASSERT_EQ_UINT(outHdr->dstPort, conn->remotePort);
    ASSERT_EQ_UINT(outHdr->seq, conn->sndNxt);
    ASSERT_EQ_UINT(outHdr->ack, conn->rcvNxt);
    ASSERT_EQ_HEX8(outHdr->flags, TCP_ACK);
    free(outPkt);

    ASSERT_EQ_UINT(conn->mslWait, g_pitTicks + 2 * TCP_MSL);

    ExitState(conn, TCP_TIME_WAIT);

    TestCaseEnd();

    return EXIT_SUCCESS;
}
