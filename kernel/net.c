// ------------------------------------------------------------------------------------------------
// net.c
// ------------------------------------------------------------------------------------------------

#include "net.h"
#include "arp.h"
#include "eth.h"
#include "ipv4.h"
#include "ipv6.h"
#include "net.h"
#include "net_driver.h"

// ------------------------------------------------------------------------------------------------
u8 net_trace = 0;
Eth_Addr net_broadcast_mac = { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };
Eth_Addr net_local_mac;
IPv4_Addr net_local_ip = { { 192, 168, 1, 42 } };
IPv4_Addr net_subnet_mask = { { 255, 255, 255, 0 } };
IPv4_Addr net_gateway_ip = { { 192, 168, 1, 1 } };

// ------------------------------------------------------------------------------------------------
void net_init()
{
    if (!net_driver.active)
    {
        return;
    }

    arp_init();
    arp_request(&net_local_ip);
}

// ------------------------------------------------------------------------------------------------
void net_poll()
{
    net_driver.poll();
}

// ------------------------------------------------------------------------------------------------
void net_rx(u8* pkt, uint len)
{
    Eth_Packet ep;
    if (!eth_decode(&ep, pkt, len))
    {
        // Bad packet or one we don't care about (e.g. STP packets)
        return;
    }

    if (net_trace)
    {
        eth_print(&ep);
    }

    // Dispatch packet based on protocol
    switch (ep.ether_type)
    {
    case ET_ARP:
        arp_rx(ep.data, ep.data_len);
        break;

    case ET_IPV4:
        ipv4_rx(ep.data, ep.data_len);
        break;

    case ET_IPV6:
        ipv6_rx(ep.data, ep.data_len);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void net_tx(u8* pkt, uint len)
{
    if (net_trace)
    {
        Eth_Packet ep;
        if (eth_decode(&ep, pkt, len))
        {
            eth_print(&ep);
        }
    }

    net_driver.tx(pkt, len);
}
