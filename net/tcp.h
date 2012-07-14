// ------------------------------------------------------------------------------------------------
// net/tcp.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/intf.h"

// ------------------------------------------------------------------------------------------------
// Configuration

#define TCP_WINDOW_SIZE     8192

// ------------------------------------------------------------------------------------------------
// Sequence comparisons

#define SEQ_LT(x,y) ((int)((x)-(y)) < 0)
#define SEQ_LE(x,y) ((int)((x)-(y)) <= 0)
#define SEQ_GT(x,y) ((int)((x)-(y)) > 0)
#define SEQ_GE(x,y) ((int)((x)-(y)) >= 0)

// ------------------------------------------------------------------------------------------------
// TCP Header

typedef struct TCP_Header
{
    u16 src_port;
    u16 dst_port;
    u32 seq;
    u32 ack;
    u8 off;
    u8 flags;
    u16 window_size;
    u16 checksum;
    u16 urgent;
} PACKED TCP_Header;

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

typedef struct TCP_Options
{
    u16 mss;
} TCP_Options;

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

// ------------------------------------------------------------------------------------------------
// TCP Connection

typedef struct TCP_Conn
{
    Link link;
    uint state;
    Net_Intf* intf;

    IPv4_Addr local_addr;
    IPv4_Addr next_addr;
    IPv4_Addr remote_addr;
    u16 local_port;
    u16 remote_port;

    // send state
    u32 snd_una;                        // send unacknowledged
    u32 snd_nxt;                        // send next
    u32 snd_wnd;                        // send window
    u32 snd_up;                         // send urgent pointer
    u32 snd_wl1;                        // segment sequence number used for last window update
    u32 snd_wl2;                        // segment acknowledgment number used for last window update
    u32 iss;                            // initial send sequence number

    // receive state
    u32 rcv_nxt;                        // receive next
    u32 rcv_wnd;                        // receive window
    u32 rcv_up;                         // receive urgent pointer
    u32 irs;                            // initial receive sequence number

    // callbacks
    void (*on_error)(const char* msg);
} TCP_Conn;

// ------------------------------------------------------------------------------------------------
// Functions

void tcp_init();
void tcp_rx(Net_Intf* intf, u8* pkt, u8* end);
void tcp_swap(TCP_Header* hdr);

TCP_Conn* tcp_create();
bool tcp_connect(TCP_Conn* conn, const IPv4_Addr* addr, u16 port);
void tcp_close(TCP_Conn* conn);
void tcp_send(TCP_Conn* conn, const void* data, uint count);
uint tcp_recv(TCP_Conn* conn, void* data, uint count);

// half-close, abort?
