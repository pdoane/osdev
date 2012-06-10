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
void eth_intf_init(Net_Intf* intf)
{
    console_init("eth_intf_init\n");
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
        ipv6_rx(ep.data, ep.data_len);
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void eth_tx(Net_Intf* intf, const Eth_Addr* dst_addr, u16 ether_type, u8* buf, uint len)
{
    u8* pkt = buf;

    Eth_Header* hdr = (Eth_Header*)pkt;
    hdr->dst = *dst_addr;
    hdr->src = intf->eth_addr;
    hdr->ether_type = net_swap16(ether_type);

    if (net_trace)
    {
        Eth_Packet ep;
        if (eth_decode(&ep, pkt, len))
        {
            eth_print(&ep);
        }
    }

    intf->tx(intf, pkt, len);
}

// ------------------------------------------------------------------------------------------------
void eth_tx_ipv4(Net_Intf* intf, const IPv4_Addr* dst_addr, u8* buf, uint len)
{
    const Eth_Addr* dst_eth_addr = arp_lookup_mac(dst_addr);
    if (!dst_eth_addr)
    {
        char dst_addr_str[IPV4_ADDR_STRING_SIZE];

        ipv4_addr_to_str(dst_addr_str, sizeof(dst_addr_str), dst_addr);
        console_print(" Unknown IP %s, sending ARP request\n", dst_addr_str);
        arp_request(intf, dst_addr);
        return;
    }

    eth_tx(intf, dst_eth_addr, ET_IPV4, buf, len);
}

// ------------------------------------------------------------------------------------------------
void eth_print(const Eth_Packet* ep)
{
    char dst_str[ETH_ADDR_STRING_SIZE];
    char src_str[ETH_ADDR_STRING_SIZE];

    eth_addr_to_str(dst_str, sizeof(dst_str), &ep->hdr->dst);
    eth_addr_to_str(src_str, sizeof(src_str), &ep->hdr->src);

    console_print("ETH: dst=%s src=%s et=%04x\n", dst_str, src_str, ep->ether_type);
}
