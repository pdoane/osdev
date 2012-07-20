// ------------------------------------------------------------------------------------------------
// net/arp.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/addr.h"
#include "net/intf.h"

// ------------------------------------------------------------------------------------------------
// ARP Header

typedef struct ARP_Header
{
    u16 htype;
    u16 ptype;
    u8 hlen;
    u8 plen;
    u16 op;
} PACKED ARP_Header;

// ------------------------------------------------------------------------------------------------
// Functions

void arp_init();

const Eth_Addr* arp_lookup_mac(const IPv4_Addr* pa);
void arp_request(Net_Intf* intf, const IPv4_Addr* tpa, u16 ether_type, Net_Buf* pkt);
void arp_reply(Net_Intf* intf, const Eth_Addr* tha, const IPv4_Addr* tpa);

void arp_rx(Net_Intf* intf, Net_Buf* pkt);
