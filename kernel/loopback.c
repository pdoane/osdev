// ------------------------------------------------------------------------------------------------
// loopback.c
// ------------------------------------------------------------------------------------------------

#include "loopback.h"
#include "arp.h"
#include "eth.h"
#include "ipv4.h"
#include "ipv6.h"
#include "net_intf.h"

// ------------------------------------------------------------------------------------------------
static void loop_init(Net_Intf* intf)
{
}

// ------------------------------------------------------------------------------------------------
static void loop_poll(Net_Intf* intf)
{
}

// ------------------------------------------------------------------------------------------------
static void loop_tx(Net_Intf* intf, const void* dst_addr, u16 ether_type, u8* pkt, uint len)
{
    // Route packet by protocol
    switch (ether_type)
    {
    case ET_ARP:
        arp_rx(intf, pkt, len);
        break;

    case ET_IPV4:
        ipv4_rx(intf, pkt, len);
        break;

    case ET_IPV6:
        ipv6_rx(intf, pkt, len);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void loopback_init()
{
    IPv4_Addr ip_addr = { { { 127, 0, 0, 1 } } };
    IPv4_Addr subnet_mask = { { { 255, 255, 255, 255 } } };

    // Create net interface
    Net_Intf* intf = net_intf_create();
    intf->eth_addr = null_eth_addr;
    intf->ip_addr = ip_addr;
    intf->name = "loop";
    intf->init = loop_init;
    intf->poll = loop_poll;
    intf->tx = loop_tx;
    intf->dev_tx = 0;

    net_intf_add(intf);

    // Add routing entries
    ipv4_add_route(&ip_addr, &subnet_mask, 0, intf);
}
