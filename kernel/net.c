// ------------------------------------------------------------------------------------------------
// net.c
// ------------------------------------------------------------------------------------------------

#include "net.h"
#include "console.h"
#include "arp.h"

// ------------------------------------------------------------------------------------------------
void net_init()
{
    arp_init();

    // Initialize interfaces
    Link* it = g_net_intf_list.next;
    Link* end = &g_net_intf_list;

    while (it != end)
    {
        Net_Intf* intf = link_data(it, Net_Intf, link);

        intf->init(intf);

        it = intf->link.next;
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

        it = intf->link.next;
    }
}
