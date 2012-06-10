// ------------------------------------------------------------------------------------------------
// eth.h
// ------------------------------------------------------------------------------------------------

#include "eth.h"
#include "arp.h"
#include "console.h"
#include "net.h"
#include "ipv4.h"
#include "ipv6.h"

// ------------------------------------------------------------------------------------------------
static bool eth_decode(Eth_Packet* ep, const u8* pkt, uint len)
{
    // Decode header
    if (len < sizeof(Eth_Header))
    {
        return false;
    }

    const Eth_Header* hdr = (const Eth_Header*)pkt;
    ep->hdr = hdr;

    // Determine which frame type is being used.
    u16 n = net_swap16(hdr->ether_type);
    if (n <= 1500 && len >= 22)
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
        ep->data = pkt + sizeof(Eth_Header);
        ep->data_len = len - sizeof(Eth_Header);
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
void eth_init_intf(Net_Intf* intf)
{
    // Broadcast gratiuitous ARP
    arp_request(intf, &intf->ip_addr);
}

// ------------------------------------------------------------------------------------------------
void eth_rx(Net_Intf* intf, u8* pkt, uint len)
{
    Eth_Packet ep;
    if (!eth_decode(&ep, pkt, len))
    {
        // Bad packet or one we don't care about (e.g. STP packets)
        return;
    }

    if (net_trace)
    {
        eth_print(&ep);
    }

    // Dispatch packet based on protocol
    switch (ep.ether_type)
    {
    case ET_ARP:
        arp_rx(intf, ep.data, ep.data_len);
        break;

    case ET_IPV4:
        ipv4_rx(intf, ep.data, ep.data_len);
        break;

    case ET_IPV6:
        ipv6_rx(intf, ep.data, ep.data_len);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void eth_tx_intf(Net_Intf* intf, const void* dst_addr, u16 ether_type, u8* pkt, uint len)
{
    // Determine ethernet address by protocol of packet
    const Eth_Addr* dst_eth_addr = 0;

    switch (ether_type)
    {
    case ET_ARP:
        dst_eth_addr = (const Eth_Addr*)dst_addr;
        break;

    case ET_IPV4:
        dst_eth_addr = arp_lookup_mac(dst_addr);
        if (!dst_eth_addr)
        {
            char dst_addr_str[IPV4_ADDR_STRING_SIZE];

            ipv4_addr_to_str(dst_addr_str, sizeof(dst_addr_str), dst_addr);
            console_print(" Unknown IP %s, sending ARP request\n", dst_addr_str);
            arp_request(intf, dst_addr);
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
    len += sizeof(Eth_Header);

    Eth_Header* hdr = (Eth_Header*)pkt;
    hdr->dst = *dst_eth_addr;
    hdr->src = intf->eth_addr;
    hdr->ether_type = net_swap16(ether_type);

    // Trace
    if (net_trace)
    {
        Eth_Packet ep;
        if (eth_decode(&ep, pkt, len))
        {
            eth_print(&ep);
        }
    }

    // Transmit
    intf->dev_tx(pkt, len);
}

// ------------------------------------------------------------------------------------------------
void eth_print(const Eth_Packet* ep)
{
    char dst_str[ETH_ADDR_STRING_SIZE];
    char src_str[ETH_ADDR_STRING_SIZE];

    eth_addr_to_str(dst_str, sizeof(dst_str), &ep->hdr->dst);
    eth_addr_to_str(src_str, sizeof(src_str), &ep->hdr->src);

    console_print("ETH: dst=%s src=%s et=%04x len=%d\n", dst_str, src_str, ep->ether_type, ep->data_len);
}
