// ------------------------------------------------------------------------------------------------
// net/addr.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
// Ethernet Address

typedef struct EthAddr
{
    u8 n[6];
} PACKED EthAddr;

#define ETH_ADDR_STRING_SIZE            18

// ------------------------------------------------------------------------------------------------
// IPv4 Address

typedef struct Ipv4Addr
{
    union
    {
        u8 n[4];
        u32 bits;
    } u;
} PACKED Ipv4Addr;

#define IPV4_ADDR_STRING_SIZE           16
#define IPV4_ADDR_PORT_STRING_SIZE      22

// ------------------------------------------------------------------------------------------------
// Globals

extern const EthAddr g_nullEthAddr;
extern const EthAddr g_broadcastEthAddr;

extern const Ipv4Addr g_nullIpv4Addr;
extern const Ipv4Addr g_broadcastIpv4Addr;

// ------------------------------------------------------------------------------------------------
// Functions

bool EthAddrEq(const EthAddr *x, const EthAddr *y);
bool Ipv4AddrEq(const Ipv4Addr *x, const Ipv4Addr *y);

void EthAddrToStr(char *str, size_t size, const EthAddr *addr);
void Ipv4AddrToStr(char *str, size_t size, const Ipv4Addr *addr);
void Ipv4AddrPortToStr(char *str, size_t size, const Ipv4Addr *addr, u16 port);

bool StrToIpv4Addr(Ipv4Addr *addr, const char *str);
bool StrToIpv4AddrPort(Ipv4Addr *addr, const char *str, u16 *port);
