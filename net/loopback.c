// ------------------------------------------------------------------------------------------------
// net/loopback.c
// ------------------------------------------------------------------------------------------------

#include "net/loopback.h"
#include "net/arp.h"
#include "net/eth.h"
#include "net/ipv4.h"
#include "net/ipv6.h"
#include "net/intf.h"
#include "net/route.h"

// ------------------------------------------------------------------------------------------------
static void loop_poll(Net_Intf* intf)
{
}

// ------------------------------------------------------------------------------------------------
static void loop_tx(Net_Intf* intf, const void* dst_addr, u16 ether_type, u8* pkt, u8* end)
{
    // Route packet by protocol
    switch (ether_type)
    {
    case ET_ARP:
        arp_rx(intf, pkt, end);
        break;

    case ET_IPV4:
        ipv4_rx(intf, pkt, end);
        break;

    case ET_IPV6:
        ipv6_rx(intf, pkt, end);
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
    intf->poll = loop_poll;
    intf->tx = loop_tx;
    intf->dev_tx = 0;

    net_intf_add(intf);

    // Add routing entries
    net_add_route(&ip_addr, &subnet_mask, 0, intf);
}
