// ------------------------------------------------------------------------------------------------
// eth.h
// ------------------------------------------------------------------------------------------------

#include "eth.h"
#include "console.h"
#include "format.h"

// ------------------------------------------------------------------------------------------------
bool eth_decode(Eth_Packet* ep, const u8* pkt, uint len)
{
    // Reject small packets
    if (len < 60)
    {
        console_print("ETH: small packet\n");
        return false;
    }

    // Source and destination addresses
    ep->dst_addr = (const Eth_Addr*)(pkt);
    ep->src_addr = (const Eth_Addr*)(pkt + 6);

    // Determine which frame type is being used.
    u16 n = (pkt[12] << 8) | pkt[13];
    if (n <= 1500)
    {
        // 802.2/802.3 encapsulation (RFC 1042)
        u8 dsap = pkt[14];
        u8 ssap = pkt[15];

        // Validate service access point
        if (dsap != 0xaa || ssap != 0xaa)
        {
            return false;
        }

        ep->ether_type = (pkt[20] << 8) | pkt[21];
        ep->data = pkt + 22;
        ep->data_len = len - 22;
    }
    else
    {
        // Ethernet encapsulation (RFC 894)
        ep->ether_type = n;
        ep->data = pkt + 14;
        ep->data_len = len - 14;
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
u8* eth_encode_hdr(u8* pkt, const Eth_Addr* dst_mac, const Eth_Addr* src_mac, u16 ether_type)
{
    *(Eth_Addr*)pkt = *dst_mac;
    *(Eth_Addr*)(pkt + 6) = *src_mac;
    pkt[12] = (ether_type >> 8) & 0xff;
    pkt[13] = (ether_type) & 0xff;
    return pkt + 14;
}

// ------------------------------------------------------------------------------------------------
void eth_addr_to_str(char* str, size_t size, const Eth_Addr* addr)
{
    snprintf(str, size, "%02x:%02x:%02x:%02x:%02x:%02x",
            addr->n[0], addr->n[1], addr->n[2], addr->n[3], addr->n[4], addr->n[5]);
}

// ------------------------------------------------------------------------------------------------
void eth_print(const Eth_Packet* ep)
{
    char dst_str[18];
    char src_str[18];

    eth_addr_to_str(dst_str, sizeof(dst_str), ep->dst_addr);
    eth_addr_to_str(src_str, sizeof(src_str), ep->src_addr);

    console_print("ETH: dst=%s src=%s et=%04x\n", dst_str, src_str, ep->ether_type);
}
