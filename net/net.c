// ------------------------------------------------------------------------------------------------
// net/net.c
// ------------------------------------------------------------------------------------------------

#include "net/net.h"
#include "net/arp.h"
#include "net/dhcp.h"
#include "net/loopback.h"
#include "net/tcp.h"

// ------------------------------------------------------------------------------------------------
// Globals

u8 net_trace = 0;

// ------------------------------------------------------------------------------------------------
void net_init()
{
    loopback_init();
    arp_init();
    tcp_init();

    // Initialize interfaces
    Net_Intf* intf;
    list_for_each(intf, net_intf_list, link)
    {
        // Check if interface needs IP address dynamically assigned
        if (!intf->ip_addr.u.bits)
        {
            dhcp_discover(intf);
        }
    }
}

// ------------------------------------------------------------------------------------------------
void net_poll()
{
    // Poll interfaces
    Net_Intf* intf;
    list_for_each(intf, net_intf_list, link)
    {
        intf->poll(intf);
    }

    tcp_poll();
}
