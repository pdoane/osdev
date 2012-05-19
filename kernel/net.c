// ------------------------------------------------------------------------------------------------
// net.c
// ------------------------------------------------------------------------------------------------

#include "net.h"
#include "arp.h"
#include "console.h"
#include "ipv4.h"
#include "net_driver.h"
#include "string.h"

// ------------------------------------------------------------------------------------------------
u8 net_trace = 0;
u8 net_broadcast_mac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
u8 net_local_mac[6];
u8 net_local_ip[4] = { 192, 168, 1, 42 };
u8 net_subnet_mask[4] = { 255, 255, 255, 0 };
u8 net_gateway_ip[4] = { 192, 168, 1, 1 };

// ------------------------------------------------------------------------------------------------
void net_init()
{
    if (!net_driver.active)
    {
        return;
    }

    arp_init();
    arp_request(net_local_ip);
}

// ------------------------------------------------------------------------------------------------
static void eth_print(u8* pkt, uint len)
{
    if (!net_trace)
    {
        return;
    }

    if (len < 14)
    {
        return;
    }

    u8* dst_mac = pkt;
    u8* src_mac = pkt + 6;
    u16 ether_type = (pkt[12] << 8) | pkt[13];

    console_print("ETH: dst=%02x:%02x:%02x:%02x:%02x:%02x src=%02x:%02x:%02x:%02x:%02x:%02x et=%04x\n",
            dst_mac[0], dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4], dst_mac[5],
            src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5],
            ether_type);
}

// ------------------------------------------------------------------------------------------------
void net_poll()
{
    net_driver.poll();
}

// ------------------------------------------------------------------------------------------------
void net_rx(u8* pkt, uint len)
{
    eth_print(pkt, len);

    //u8* dst_mac = pkt;
    //u8* src_mac = pkt + 6;
    u16 ether_type = (pkt[12] << 8) | pkt[13];
    pkt += 14;
    len -= 14;

    // Dispatch packet based on protocol
    switch (ether_type)
    {
    case ET_ARP:
        arp_rx(pkt, len);
        break;

    case ET_IPV4:
        ipv4_rx(pkt, len);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void net_tx(u8* pkt, uint len)
{
    eth_print(pkt, len);

    net_driver.tx(pkt, len);
}

// ------------------------------------------------------------------------------------------------
u8* net_eth_hdr(u8* pkt, u8* dst_mac, u16 ether_type)
{
    memcpy(pkt + 0, dst_mac, 6);
    memcpy(pkt + 6, net_local_mac, 6);
    pkt[12] = (ether_type >> 8) & 0xff;
    pkt[13] = (ether_type) & 0xff;
    return pkt + 14;
}
