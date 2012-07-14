// ------------------------------------------------------------------------------------------------
// net/route.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/intf.h"

// ------------------------------------------------------------------------------------------------
// Route Entry

typedef struct Net_Route
{
    Link link;
    IPv4_Addr dst;
    IPv4_Addr mask;
    IPv4_Addr gateway;
    Net_Intf* intf;
} Net_Route;

// ------------------------------------------------------------------------------------------------
// Functions

const Net_Route* net_find_route(const IPv4_Addr* dst);
void net_add_route(const IPv4_Addr* dst, const IPv4_Addr* mask, const IPv4_Addr* gateway, Net_Intf* intf);
const IPv4_Addr* net_next_addr(const Net_Route* route, const IPv4_Addr* dst_addr);
void net_print_route_table();
