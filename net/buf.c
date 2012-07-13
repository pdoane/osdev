// ------------------------------------------------------------------------------------------------
// net/buf.c
// ------------------------------------------------------------------------------------------------

#include "net/buf.h"
#include "mem/vm.h"

// ------------------------------------------------------------------------------------------------
static Link net_free_packets = { &net_free_packets, &net_free_packets };

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
