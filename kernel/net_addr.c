// ------------------------------------------------------------------------------------------------
// net_addr.c
// ------------------------------------------------------------------------------------------------

#include "net_addr.h"
#include "format.h"

// ------------------------------------------------------------------------------------------------
const Eth_Addr null_eth_addr            = { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };
const Eth_Addr broadcast_eth_addr       = { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };

const IPv4_Addr null_ipv4_addr          = { { { 0x00, 0x00, 0x00, 0x00 } } };
const IPv4_Addr broadcast_ipv4_addr     = { { { 0xff, 0xff, 0xff, 0xff } } };

// ------------------------------------------------------------------------------------------------
bool eth_addr_eq(const Eth_Addr* x, const Eth_Addr* y)
{
    for (uint i = 0; i < 6; ++i)
    {
        if (x->n[i] != y->n[i])
        {
            return false;
        }
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
bool ipv4_addr_eq(const IPv4_Addr* x, const IPv4_Addr* y)
{
    return x->u.bits == y->u.bits;
}

// ------------------------------------------------------------------------------------------------
void eth_addr_to_str(char* str, size_t size, const Eth_Addr* addr)
{
    snprintf(str, size, "%02x:%02x:%02x:%02x:%02x:%02x",
            addr->n[0], addr->n[1], addr->n[2], addr->n[3], addr->n[4], addr->n[5]);
}

// ------------------------------------------------------------------------------------------------
void ipv4_addr_to_str(char* str, size_t size, const IPv4_Addr* addr)
{
    snprintf(str, size, "%d.%d.%d.%d", addr->u.n[0], addr->u.n[1], addr->u.n[2], addr->u.n[3]);
}

// ------------------------------------------------------------------------------------------------
bool str_to_ipv4_addr(IPv4_Addr* addr, const char* str)
{
    return sscanf(str, "%d.%d.%d.%d", &addr->u.n[0], &addr->u.n[1], &addr->u.n[2], &addr->u.n[3]) == 4;
}
