// ------------------------------------------------------------------------------------------------
// net.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "eth.h"
#include "ipv4.h"

// ------------------------------------------------------------------------------------------------
// Globals

extern u8 net_trace;
extern Eth_Addr net_broadcast_mac;
extern Eth_Addr net_local_mac;
extern IPv4_Addr net_local_ip;
extern IPv4_Addr net_subnet_mask;
extern IPv4_Addr net_gateway_ip;

// ------------------------------------------------------------------------------------------------

void net_init();

void net_poll();
void net_rx(u8* pkt, uint len);
void net_tx(u8* pkt, uint len);
