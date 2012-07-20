// ------------------------------------------------------------------------------------------------
// net/intf.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/addr.h"
#include "net/buf.h"
#include "stdlib/link.h"

// ------------------------------------------------------------------------------------------------
// Net Interface

typedef struct Net_Intf
{
    Link link;
    Eth_Addr eth_addr;
    IPv4_Addr ip_addr;
    IPv4_Addr broadcast_addr;
    const char* name;

    void (*poll)(struct Net_Intf* intf);
    void (*tx)(struct Net_Intf* intf, const void* dst_addr, u16 ether_type, Net_Buf* buf);
    void (*dev_tx)(Net_Buf* buf);
} Net_Intf;

// ------------------------------------------------------------------------------------------------
// Globals

extern Link net_intf_list;

// ------------------------------------------------------------------------------------------------
// Functions

Net_Intf* net_intf_create();
void net_intf_add(Net_Intf* intf);
