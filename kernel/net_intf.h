// ------------------------------------------------------------------------------------------------
// net_intf.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "link.h"
#include "net_addr.h"

// ------------------------------------------------------------------------------------------------
// Net Interface

typedef struct Net_Intf
{
    Link link;
    Eth_Addr eth_addr;
    IPv4_Addr ip_addr;
    const char* name;

    void (*poll)(struct Net_Intf* intf);
    void (*tx)(struct Net_Intf* intf, const void* dst_addr, u16 ether_type, u8* pkt, uint len);
    void (*dev_tx)(u8* pkt, uint len);
} Net_Intf;

// ------------------------------------------------------------------------------------------------
// Globals

extern Link g_net_intf_list;

// ------------------------------------------------------------------------------------------------
// Functions

Net_Intf* net_intf_create();
void net_intf_add(Net_Intf* intf);
