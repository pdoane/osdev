// ------------------------------------------------------------------------------------------------
// net/icmp.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/intf.h"

void icmp_rx(Net_Intf* intf, u8* pkt, u8* end);

void icmp_echo_request(const IPv4_Addr* dst_addr, u16 id, u16 sequence,
    const u8* data, const u8* end);
