// ------------------------------------------------------------------------------------------------
// net/icmp.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/ipv4.h"

void icmp_rx(Net_Intf* intf, const IPv4_Header* ip_hdr, Net_Buf* pkt);

void icmp_echo_request(const IPv4_Addr* dst_addr, u16 id, u16 sequence,
    const u8* data, const u8* end);
