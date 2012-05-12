// ------------------------------------------------------------------------------------------------
// net.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"

// ------------------------------------------------------------------------------------------------
// Ethertypes
#define ET_IPV4                         0x0800
#define ET_ARP                          0x0806
#define ET_IPV6                         0x86DD

// ------------------------------------------------------------------------------------------------
// Globals
extern u8 net_trace;
extern u8 net_broadcast_mac[6];
extern u8 net_local_mac[6];
extern u8 net_local_ip[4];
extern u8 net_subnet_mask[4];
extern u8 net_gateway_ip[4];

// ------------------------------------------------------------------------------------------------
// Functions
void net_init();

void net_poll();
void net_rx(u8* pkt, uint len);
void net_tx(u8* pkt, uint len);

u8* net_eth_hdr(u8* pkt, u8* dst_mac, u16 ether_type);
