// ------------------------------------------------------------------------------------------------
// net/eth.h
// ------------------------------------------------------------------------------------------------

#include "net/eth.h"
#include "net/arp.h"
#include "net/ipv4.h"
#include "net/ipv6.h"
#include "net/net.h"
#include "net/swap.h"
#include "console/console.h"

// ------------------------------------------------------------------------------------------------
static bool eth_decode(Eth_Packet* ep, u8* pkt, u8* end)
{
    // Decode header
    if (pkt + sizeof(Eth_Header) > end)
    {
        return false;
    }

    Eth_Header* hdr = (Eth_Header*)pkt;
    ep->hdr = hdr;

    // Determine which frame type is being used.
    u16 n = net_swap16(hdr->ether_type);
    if (n <= 1500 && pkt + 22 <= end)
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
    }
    else
    {
        // Ethernet encapsulation (RFC 894)
        ep->ether_type = n;
        ep->data = pkt + sizeof(Eth_Header);
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
void eth_rx(Net_Intf* intf, u8* pkt, u8* end)
{
    Eth_Packet ep;
    if (!eth_decode(&ep, pkt, end))
    {
        // Bad packet or one we don't care about (e.g. STP packets)
        return;
    }

    eth_print(pkt, end);

    // Dispatch packet based on protocol
    switch (ep.ether_type)
    {
    case ET_ARP:
        arp_rx(intf, ep.data, end);
        break;

    case ET_IPV4:
        ipv4_rx(intf, ep.data, end);
        break;

    case ET_IPV6:
        ipv6_rx(intf, ep.data, end);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void eth_tx_intf(Net_Intf* intf, const void* dst_addr, u16 ether_type, u8* pkt, u8* end)
{
    // Determine ethernet address by protocol of packet
    const Eth_Addr* dst_eth_addr = 0;

    switch (ether_type)
    {
    case ET_ARP:
        dst_eth_addr = (const Eth_Addr*)dst_addr;
        break;

    case ET_IPV4:
        {
            const IPv4_Addr* dst_ipv4_addr = (const IPv4_Addr*)dst_addr;

            if (ipv4_addr_eq(dst_ipv4_addr, &broadcast_ipv4_addr) ||
                ipv4_addr_eq(dst_ipv4_addr, &intf->broadcast_addr))
            {
                // IP Broadcast -> Ethernet Broacast
                dst_eth_addr = &broadcast_eth_addr;
            }
            else
            {
                // Lookup Ethernet address in ARP cache
                dst_eth_addr = arp_lookup_mac(dst_ipv4_addr);
                if (!dst_eth_addr)
                {
                    arp_request(intf, dst_ipv4_addr, ether_type, pkt, end);
                    return;
                }
            }
        }
        break;

    case ET_IPV6:
        break;
    }

    // Skip packets without a destination
    if (!dst_eth_addr)
    {
        console_print("Dropped packet\n");
        return;
    }

    // Fill in ethernet header
    pkt -= sizeof(Eth_Header);

    Eth_Header* hdr = (Eth_Header*)pkt;
    hdr->dst = *dst_eth_addr;
    hdr->src = intf->eth_addr;
    hdr->ether_type = net_swap16(ether_type);

    // Transmit
    eth_print(pkt, end);
    intf->dev_tx(pkt, end);
}

// ------------------------------------------------------------------------------------------------
void eth_print(u8* pkt, u8* end)
{
    if (~net_trace & TRACE_LINK)
    {
        return;
    }

    Eth_Packet ep;
    if (eth_decode(&ep, pkt, end))
    {
        char dst_str[ETH_ADDR_STRING_SIZE];
        char src_str[ETH_ADDR_STRING_SIZE];

        eth_addr_to_str(dst_str, sizeof(dst_str), &ep.hdr->dst);
        eth_addr_to_str(src_str, sizeof(src_str), &ep.hdr->src);

        uint len = end - ep.data;
        console_print("ETH: dst=%s src=%s et=%04x len=%d\n", dst_str, src_str, ep.ether_type, len);
    }
}
