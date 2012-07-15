// ------------------------------------------------------------------------------------------------
// net/addr.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
// Ethernet Address

typedef struct Eth_Addr
{
    u8 n[6];
} PACKED Eth_Addr;

#define ETH_ADDR_STRING_SIZE            18

// ------------------------------------------------------------------------------------------------
// IPv4 Address

typedef struct IPv4_Addr
{
    union
    {
        u8 n[4];
        u32 bits;
    } u;
} PACKED IPv4_Addr;

#define IPV4_ADDR_STRING_SIZE           16
#define IPV4_ADDR_PORT_STRING_SIZE      22

// ------------------------------------------------------------------------------------------------
// Globals

extern const Eth_Addr null_eth_addr;
extern const Eth_Addr broadcast_eth_addr;

extern const IPv4_Addr null_ipv4_addr;
extern const IPv4_Addr broadcast_ipv4_addr;

// ------------------------------------------------------------------------------------------------
// Functions

bool eth_addr_eq(const Eth_Addr* x, const Eth_Addr* y);
bool ipv4_addr_eq(const IPv4_Addr* x, const IPv4_Addr* y);

void eth_addr_to_str(char* str, size_t size, const Eth_Addr* addr);
void ipv4_addr_to_str(char* str, size_t size, const IPv4_Addr* addr);
void ipv4_addr_port_to_str(char* str, size_t size, const IPv4_Addr* addr, u16 port);

bool str_to_ipv4_addr(IPv4_Addr* addr, const char* str);
bool str_to_ipv4_addr_port(IPv4_Addr* addr, const char* str, u16* port);
