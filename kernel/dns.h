// ------------------------------------------------------------------------------------------------
// dns.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net_intf.h"

// ------------------------------------------------------------------------------------------------

void dns_rx(Net_Intf* intf, const u8* pkt, uint len);
void dns_query_host(const IPv4_Addr* dns_addr, const char* host, uint id);

void dns_print(const u8* pkt, uint len);
