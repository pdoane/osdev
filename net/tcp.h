// ------------------------------------------------------------------------------------------------
// net/tcp.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/ipv4.h"

// ------------------------------------------------------------------------------------------------
// Configuration

#define TCP_WINDOW_SIZE     8192
#define TCP_MSL             120000      // Maximum Segment Lifetime (ms)

// ------------------------------------------------------------------------------------------------
// Sequence comparisons

#define SEQ_LT(x,y) ((int)((x)-(y)) < 0)
#define SEQ_LE(x,y) ((int)((x)-(y)) <= 0)
#define SEQ_GT(x,y) ((int)((x)-(y)) > 0)
#define SEQ_GE(x,y) ((int)((x)-(y)) >= 0)

// ------------------------------------------------------------------------------------------------
// TCP Header

typedef struct TcpHeader
{
    u16 srcPort;
    u16 dstPort;
    u32 seq;
    u32 ack;
    u8 off;
    u8 flags;
    u16 windowSize;
    u16 checksum;
    u16 urgent;
} PACKED TcpHeader;

// Flags
#define TCP_FIN                         (1 << 0)
#define TCP_SYN                         (1 << 1)
#define TCP_RST                         (1 << 2)
#define TCP_PSH                         (1 << 3)
#define TCP_ACK                         (1 << 4)
#define TCP_URG                         (1 << 5)

// ------------------------------------------------------------------------------------------------
// Options

#define OPT_END                         0
#define OPT_NOP                         1
#define OPT_MSS                         2

typedef struct TcpOptions
{
    u16 mss;
} TcpOptions;

// ------------------------------------------------------------------------------------------------
// TCP State

#define TCP_CLOSED                      0
#define TCP_LISTEN                      1
#define TCP_SYN_SENT                    2
#define TCP_SYN_RECEIVED                3
#define TCP_ESTABLISHED                 4
#define TCP_FIN_WAIT_1                  5
#define TCP_FIN_WAIT_2                  6
#define TCP_CLOSE_WAIT                  7
#define TCP_CLOSING                     8
#define TCP_LAST_ACK                    9
#define TCP_TIME_WAIT                   10

extern const char *g_tcpStateStrs[];

// ------------------------------------------------------------------------------------------------
// TCP Errors

#define TCP_CONN_RESET                  1
#define TCP_CONN_REFUSED                2
#define TCP_CONN_CLOSING                3

// ------------------------------------------------------------------------------------------------
// TCP Connection

typedef struct TcpConn
{
    Link link;
    uint state;
    NetIntf *intf;

    Ipv4Addr localAddr;
    Ipv4Addr nextAddr;
    Ipv4Addr remoteAddr;
    u16 localPort;
    u16 remotePort;

    // send state
    u32 sndUna;                         // send unacknowledged
    u32 sndNxt;                         // send next
    u32 sndWnd;                         // send window
    u32 sndUP;                          // send urgent pointer
    u32 sndWl1;                         // segment sequence number used for last window update
    u32 sndWl2;                         // segment acknowledgment number used for last window update
    u32 iss;                            // initial send sequence number

    // receive state
    u32 rcvNxt;                        // receive next
    u32 rcvWnd;                        // receive window
    u32 rcvUP;                         // receive urgent pointer
    u32 irs;                            // initial receive sequence number

    // queues
    Link resequence;

    // timers
    u32 mslWait;                       // when does the 2MSL time wait expire?

    // callbacks
    void *ctx;
    void (*onError)(struct TcpConn *conn, uint error);
    void (*onState)(struct TcpConn *conn, uint oldState, uint newState);
    void (*onData)(struct TcpConn *conn, const u8 *data, uint len);
} TcpConn;

// ------------------------------------------------------------------------------------------------
// Globals

extern Link g_tcpActiveConns;

// ------------------------------------------------------------------------------------------------
// Internal Functions

void TcpInit();
void TcpRecv(NetIntf *intf, const Ipv4Header *ipHdr, NetBuf *pkt);
void TcpPoll();
void TcpSwap(TcpHeader *hdr);

// ------------------------------------------------------------------------------------------------
// User API

TcpConn *TcpCreate();
bool TcpConnect(TcpConn *conn, const Ipv4Addr *addr, u16 port);
void TcpClose(TcpConn *conn);
void TcpSend(TcpConn *conn, const void *data, uint count);

// half-close, abort?
