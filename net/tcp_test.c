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

static IPv4_Addr ip_addr = { { { 127, 0, 0, 1 } } };
static IPv4_Addr subnet_mask = { { { 255, 255, 255, 255 } } };

// ------------------------------------------------------------------------------------------------
// Mocked dependencies

u8 net_trace;
u32 pit_ticks;
Checksum_Header phdr;
u8 pkt_data[1500];
u8* pkt_end;

void rtc_get_time(DateTime* dt)
{
    split_time(dt, 0, 0);
}

void console_print(const char* fmt, ...)
{
}

void ipv4_tx_intf(Net_Intf* intf, const IPv4_Addr* next_addr,
    const IPv4_Addr* dst_addr, u8 protocol, u8* pkt, u8* end)
{
    uint len = end - pkt;

    phdr.src = intf->ip_addr;
    phdr.dst = *dst_addr;
    phdr.protocol = protocol;
    phdr.len = net_swap16(len);

    memcpy(pkt_data, pkt, len);
    pkt_end = pkt_data + len;
}

void* vm_alloc(uint size)
{
    return malloc(size);
}

// ------------------------------------------------------------------------------------------------
static void validate_checksum()
{
    u8* phdr_data = (u8*)&phdr;
    u8* phdr_end = phdr_data + sizeof(Checksum_Header);

    uint sum = 0;
    sum = net_checksum_acc(phdr_data, phdr_end, sum);
    sum = net_checksum_acc(pkt_data, pkt_end, sum);
    u16 checksum = net_checksum_final(sum);

    ASSERT_EQ_UINT(checksum, 0);
}

// ------------------------------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    // Create net interface
    Net_Intf* intf = net_intf_create();
    intf->eth_addr = null_eth_addr;
    intf->ip_addr = ip_addr;
    intf->name = "test";
    intf->poll = 0;
    intf->tx = 0;
    intf->dev_tx = 0;

    //net_intf_add(intf);

    // Add routing entry
    net_add_route(&ip_addr, &subnet_mask, 0, intf);

    // Test
    IPv4_Addr dst_addr = { { { 127, 0, 0, 1 } } };
    u16 port = 80;

    TCP_Conn* conn = tcp_connect(&dst_addr, port);
    ASSERT_TRUE(conn);
    ASSERT_EQ_UINT(pkt_end - pkt_data, 24);
    validate_checksum();

    TCP_Header* hdr = (TCP_Header*)pkt_data;
    tcp_swap(hdr);
    ASSERT_TRUE(hdr->src_port >= 49152);
    ASSERT_EQ_UINT(hdr->dst_port, 80);
    ASSERT_EQ_UINT(hdr->ack, 0);
    ASSERT_EQ_HEX8(hdr->flags, TCP_SYN);
    ASSERT_EQ_UINT(hdr->window_size, TCP_WINDOW_SIZE);
    ASSERT_EQ_UINT(hdr->urgent, 0);

    tcp_close(conn);

    return EXIT_SUCCESS;
}
