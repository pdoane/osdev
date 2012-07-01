// ------------------------------------------------------------------------------------------------
// net/icmp.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/intf.h"

void icmp_rx(Net_Intf* intf, const u8* pkt, uint len);

void icmp_echo_request(const IPv4_Addr* dst_addr, u16 id, u16 sequence,
    const u8* echo_data, uint echo_data_len);
