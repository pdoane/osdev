// ------------------------------------------------------------------------------------------------
// arp.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"

void arp_init();

u8* arp_lookup_mac(u8* pa);
void arp_request(u8* tpa);
void arp_reply(u8* tha, u8* tpa);

void arp_rx(u8* pkt, uint len);
