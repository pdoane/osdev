// ------------------------------------------------------------------------------------------------
// net/net.c
// ------------------------------------------------------------------------------------------------

#include "net/net.h"
#include "net/arp.h"
#include "net/dhcp.h"
#include "net/loopback.h"
#include "console/console.h"
#include "mem/vm.h"

// ------------------------------------------------------------------------------------------------

static Link net_free_packets = { &net_free_packets, &net_free_packets };

u8 net_trace = 0;

// ------------------------------------------------------------------------------------------------
void net_init()
{
    loopback_init();
    arp_init();

    // Initialize interfaces
    Link* it = g_net_intf_list.next;
    Link* end = &g_net_intf_list;

    while (it != end)
    {
        Net_Intf* intf = link_data(it, Net_Intf, link);

        // Check if interface needs IP address dynamically assigned
        if (!intf->ip_addr.u.bits)
        {
            dhcp_discover(intf);
        }

        it = it->next;
    }
}

// ------------------------------------------------------------------------------------------------
void net_poll()
{
    // Poll interfaces
    Link* it = g_net_intf_list.next;
    Link* end = &g_net_intf_list;

    while (it != end)
    {
        Net_Intf* intf = link_data(it, Net_Intf, link);

        intf->poll(intf);

        it = it->next;
    }
}

// ------------------------------------------------------------------------------------------------
NetBuf* net_alloc_packet()
{
    Link* p = net_free_packets.next;
    if (p != &net_free_packets)
    {
        link_remove(p);
        return link_data(p, NetBuf, link);
    }
    else
    {
        return vm_alloc(MAX_PACKET_SIZE);
    }
}

// ------------------------------------------------------------------------------------------------
void net_free_packet(NetBuf* buf)
{
    link_before(&net_free_packets, &buf->link);
}
