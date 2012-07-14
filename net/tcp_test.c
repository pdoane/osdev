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

static Net_Intf* intf;
static IPv4_Addr ip_addr = { { { 127, 0, 0, 1 } } };
static IPv4_Addr subnet_mask = { { { 255, 255, 255, 255 } } };

// ------------------------------------------------------------------------------------------------
// Packets

typedef struct Packet
{
    Link link;
    Checksum_Header phdr;
    u8 data[1500];
    u8* end;
} Packet;

// ------------------------------------------------------------------------------------------------
// Mocked dependencies

u8 net_trace;
u32 pit_ticks;
Link out_packets = { &out_packets, &out_packets };

void rtc_get_time(DateTime* dt)
{
    split_time(dt, 0, 0);
}

void console_print(const char* fmt, ...)
{
    /*
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    */
}

void ipv4_tx_intf(Net_Intf* intf, const IPv4_Addr* next_addr,
    const IPv4_Addr* dst_addr, u8 protocol, u8* pkt, u8* end)
{
    uint len = end - pkt;

    Packet* packet = malloc(sizeof(Packet));
    packet->phdr.src = intf->ip_addr;
    packet->phdr.dst = *dst_addr;
    packet->phdr.protocol = protocol;
    packet->phdr.len = net_swap16(len);

    memcpy(packet->data, pkt, len);
    packet->end = packet->data + len;

    link_before(&out_packets, &packet->link);
}

void* vm_alloc(uint size)
{
    return malloc(size);
}

// ------------------------------------------------------------------------------------------------
static void tcp_input(u8* pkt)
{
    TCP_Header* tcp_hdr = (TCP_Header*)pkt;
    tcp_swap(tcp_hdr);

    // Data
    u8* end = pkt + sizeof(TCP_Header);

    // Pseudo Header
    Checksum_Header* phdr = (Checksum_Header*)(pkt - sizeof(Checksum_Header));
    phdr->src = ip_addr;
    phdr->dst = ip_addr;
    phdr->reserved = 0;
    phdr->protocol = IP_PROTOCOL_TCP;
    phdr->len = net_swap16(end - pkt);

    // Checksum
    u16 checksum = net_checksum(pkt - sizeof(Checksum_Header), end);
    tcp_hdr->checksum = net_swap16(checksum);

    // IP Header
    pkt -= sizeof(IPv4_Header);

    IPv4_Header* ip_hdr = (IPv4_Header*)pkt;
    ip_hdr->ver_ihl = (4 << 4) | 5;
    ip_hdr->tos = 0;
    ip_hdr->len = net_swap16(end - pkt);
    ip_hdr->id = net_swap16(0);
    ip_hdr->offset = net_swap16(0);
    ip_hdr->ttl = 64;
    ip_hdr->protocol = IP_PROTOCOL_TCP;
    ip_hdr->checksum = 0;
    ip_hdr->src = ip_addr;
    ip_hdr->dst = ip_addr;

    // Receive
    tcp_rx(intf, pkt, end);
}

// ------------------------------------------------------------------------------------------------
static void validate_checksum(Packet* pkt)
{
    u8* phdr_data = (u8*)&pkt->phdr;
    u8* phdr_end = phdr_data + sizeof(Checksum_Header);

    uint sum = 0;
    sum = net_checksum_acc(phdr_data, phdr_end, sum);
    sum = net_checksum_acc(pkt->data, pkt->end, sum);
    u16 checksum = net_checksum_final(sum);

    ASSERT_EQ_UINT(checksum, 0);
}

// ------------------------------------------------------------------------------------------------
static Packet* pop_packet()
{
    ASSERT_TRUE(!list_empty(&out_packets));

    Packet* packet = link_data(out_packets.next, Packet, link);
    link_remove(&packet->link);

    validate_checksum(packet);
    return packet;
}

// ------------------------------------------------------------------------------------------------
static void test_case(const char* msg)
{
    printf("-- %s\n", msg);
}

// ------------------------------------------------------------------------------------------------
static void test_setup()
{
    // Create net interface
    intf = net_intf_create();
    intf->eth_addr = null_eth_addr;
    intf->ip_addr = ip_addr;
    intf->name = "test";
    intf->poll = 0;
    intf->tx = 0;
    intf->dev_tx = 0;

    //net_intf_add(intf);

    // Add routing entry
    net_add_route(&ip_addr, &subnet_mask, 0, intf);
}

// ------------------------------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    // Common variables
    u8 in_buf[2048];
    u8* in_pkt = in_buf + 256;
    Packet* out_pkt;
    TCP_Header* in_hdr;
    TCP_Header* out_hdr;
    TCP_Conn* conn;

    // --------------------------------------------------------------------------------------------
    test_setup();

    ASSERT_TRUE(list_empty(&out_packets));

    // --------------------------------------------------------------------------------------------
    in_hdr = (TCP_Header*)in_pkt;

    test_case("CLOSED: Receive RST - packet should be dropped");
    in_hdr->src_port = 100;
    in_hdr->dst_port = 101;
    in_hdr->seq = 1;
    in_hdr->ack = 2;
    in_hdr->off = 5 << 4;
    in_hdr->flags = TCP_RST;
    in_hdr->window_size = TCP_WINDOW_SIZE;
    in_hdr->checksum = 0;
    in_hdr->urgent = 0;
    tcp_input(in_pkt);
    ASSERT_TRUE(list_empty(&out_packets));

    test_case("CLOSED: Receive ACK - RST should be sent");
    in_hdr->src_port = 100;
    in_hdr->dst_port = 101;
    in_hdr->seq = 1;
    in_hdr->ack = 2;
    in_hdr->off = 5 << 4;
    in_hdr->flags = TCP_ACK;
    in_hdr->window_size = TCP_WINDOW_SIZE;
    in_hdr->checksum = 0;
    in_hdr->urgent = 0;
    tcp_input(in_pkt);

    out_pkt = pop_packet();
    out_hdr = (TCP_Header*)out_pkt->data;
    tcp_swap(out_hdr);
    ASSERT_EQ_UINT(out_hdr->src_port, 101);
    ASSERT_EQ_UINT(out_hdr->dst_port, 100);
    ASSERT_EQ_UINT(out_hdr->seq, 2);
    ASSERT_EQ_UINT(out_hdr->ack, 0);
    ASSERT_EQ_HEX8(out_hdr->flags, TCP_RST);

    ASSERT_TRUE(list_empty(&out_packets));

    test_case("CLOSED: Receive non-ACK - RST/ACK should be sent");
    in_hdr->src_port = 100;
    in_hdr->dst_port = 101;
    in_hdr->seq = 1;
    in_hdr->ack = 2;
    in_hdr->off = 5 << 4;
    in_hdr->flags = 0;
    in_hdr->window_size = TCP_WINDOW_SIZE;
    in_hdr->checksum = 0;
    in_hdr->urgent = 0;
    tcp_input(in_pkt);

    out_pkt = pop_packet();
    out_hdr = (TCP_Header*)out_pkt->data;
    tcp_swap(out_hdr);
    ASSERT_EQ_UINT(out_hdr->src_port, 101);
    ASSERT_EQ_UINT(out_hdr->dst_port, 100);
    ASSERT_EQ_UINT(out_hdr->seq, 0);
    ASSERT_EQ_UINT(out_hdr->ack, 1);
    ASSERT_EQ_HEX8(out_hdr->flags, TCP_RST | TCP_ACK);

    ASSERT_TRUE(list_empty(&out_packets));

    // --------------------------------------------------------------------------------------------
    test_case("SYN_SENT: Open connection");

    conn = tcp_create();
    ASSERT_TRUE(tcp_connect(conn, &ip_addr, 80));

    out_pkt = pop_packet();
    out_hdr = (TCP_Header*)out_pkt->data;
    tcp_swap(out_hdr);
    ASSERT_TRUE(out_hdr->src_port >= 49152);
    ASSERT_EQ_UINT(out_hdr->dst_port, 80);
    ASSERT_EQ_UINT(out_hdr->ack, 0);
    ASSERT_EQ_HEX8(out_hdr->flags, TCP_SYN);
    ASSERT_EQ_UINT(out_hdr->window_size, TCP_WINDOW_SIZE);
    ASSERT_EQ_UINT(out_hdr->urgent, 0);

    tcp_close(conn);

    return EXIT_SUCCESS;
}
