// ------------------------------------------------------------------------------------------------
// arp.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "eth.h"
#include "ipv4.h"

// ------------------------------------------------------------------------------------------------

void arp_init();

const Eth_Addr* arp_lookup_mac(const IPv4_Addr* pa);
void arp_request(const IPv4_Addr* tpa);
void arp_reply(const Eth_Addr* tha, const IPv4_Addr* tpa);

void arp_rx(const u8* pkt, uint len);
