// ------------------------------------------------------------------------------------------------
// arp.c
// ------------------------------------------------------------------------------------------------

#include "arp.h"
#include "console.h"
#include "ipv4.h"
#include "eth.h"
#include "net.h"
#include "string.h"

// ------------------------------------------------------------------------------------------------
// ARP Protocol
#define ARP_HTYPE_ETH       0x01

#define ARP_OP_REQUEST      0x01
#define ARP_OP_REPLY        0x02

// ------------------------------------------------------------------------------------------------
// ARP Cache
#define ARP_CACHE_SIZE      16

typedef struct ARP_Entry
{
    Eth_Addr ha;
    IPv4_Addr pa;
} ARP_Entry;

static ARP_Entry arp_cache[ARP_CACHE_SIZE];

// ------------------------------------------------------------------------------------------------
static void arp_print(const u8* pkt, uint len)
{
    if (!net_trace)
    {
        return;
    }

    if (len < 8)
    {
        return;
    }

    u16 htype = (pkt[0] << 8) | pkt[1];
    u16 ptype = (pkt[2] << 8) | pkt[3];
    u8 hlen = pkt[4];
    u8 plen = pkt[5];
    u16 op = (pkt[6] << 8) | pkt[7];
    console_print(" ARP: htype=0x%x ptype=0x%x hlen=%d plen=%d op=%d\n",
            htype, ptype, hlen, plen, op);

    if (htype == ARP_HTYPE_ETH && ptype == ET_IPV4 && len >= 28)
    {
        const Eth_Addr* sha = (const Eth_Addr*)(pkt + 8);
        const IPv4_Addr* spa = (const IPv4_Addr*)(pkt + 14);
        const Eth_Addr* tha = (const Eth_Addr*)(pkt + 18);
        const IPv4_Addr* tpa = (const IPv4_Addr*)(pkt + 24);

        char sha_str[18];
        char spa_str[16];
        char tha_str[18];
        char tpa_str[16];

        eth_addr_to_str(sha_str, sizeof(sha_str), sha);
        ipv4_addr_to_str(spa_str, sizeof(spa_str), spa);
        eth_addr_to_str(tha_str, sizeof(tha_str), tha);
        ipv4_addr_to_str(tpa_str, sizeof(tpa_str), tpa);

        console_print(" ARP: %s spa=%s\n", sha_str, spa_str);
        console_print(" ARP: %s tpa=%s\n", tha_str, tpa_str);
    }
}

// ------------------------------------------------------------------------------------------------
static void arp_snd(uint op, const Eth_Addr* tha, const IPv4_Addr* tpa)
{
    u8 data[256];

    u8* arp_pkt = eth_encode_hdr(data, tha, &net_local_mac, ET_ARP);

    // HTYPE
    arp_pkt[0] = (ARP_HTYPE_ETH >> 8) & 0xff;
    arp_pkt[1] = (ARP_HTYPE_ETH) & 0xff;

    // PTYPE
    arp_pkt[2] = (ET_IPV4 >> 8) & 0xff;
    arp_pkt[3] = (ET_IPV4) & 0xff;

    // HLEN
    arp_pkt[4] = sizeof(Eth_Addr);

    // PLEN
    arp_pkt[5] = sizeof(IPv4_Addr);

    // Operation
    arp_pkt[6] = (op >> 8) & 0xff;
    arp_pkt[7] = (op) & 0xff;

    // SHA
    *(Eth_Addr*)(arp_pkt + 8) = net_local_mac;

    // SPA
    *(IPv4_Addr*)(arp_pkt + 14) = net_local_ip;

    // THA
    if (op == ARP_OP_REQUEST)
    {
        Eth_Addr null = { { 0 } };
        *(Eth_Addr*)(arp_pkt + 18) = null;
    }
    else
    {
        *(Eth_Addr*)(arp_pkt + 18) = *tha;
    }

    // TPA
    *(IPv4_Addr*)(arp_pkt + 24) = *tpa;

    // Print
    arp_print(arp_pkt, 28);

    // Transmit packet
    uint len = 14 + 28;

    net_tx(data, len);
}

// ------------------------------------------------------------------------------------------------
void arp_request(const IPv4_Addr* tpa)
{
    arp_snd(ARP_OP_REQUEST, &net_broadcast_mac, tpa);
}

// ------------------------------------------------------------------------------------------------
void arp_reply(const Eth_Addr* tha, const IPv4_Addr* tpa)
{
    arp_snd(ARP_OP_REPLY, tha, tpa);
}

// ------------------------------------------------------------------------------------------------
static void arp_add(const Eth_Addr* ha, const IPv4_Addr* pa)
{
    // TODO - handle overflow
    ARP_Entry* entry = arp_cache;
    ARP_Entry* end = entry + ARP_CACHE_SIZE;
    for (; entry != end; ++entry)
    {
        if ((entry->pa.n[0] == 0 &&
             entry->pa.n[1] == 0 &&
             entry->pa.n[2] == 0 &&
             entry->pa.n[3] == 0))
        {
            break;
        }
    }

    if (entry != end)
    {
        entry->ha = *ha;
        entry->pa = *pa;
    }
}

// ------------------------------------------------------------------------------------------------
void arp_init()
{
    // Clear cache of all entries
    ARP_Entry* entry = arp_cache;
    ARP_Entry* end = entry + ARP_CACHE_SIZE;
    for (; entry != end; ++entry)
    {
        memset(&entry->ha, 0, sizeof(Eth_Addr));
        memset(&entry->pa, 0, sizeof(IPv4_Addr));
    }
}

// ------------------------------------------------------------------------------------------------
static ARP_Entry* arp_lookup(const IPv4_Addr* pa)
{
    ARP_Entry* entry = arp_cache;
    ARP_Entry* end = entry + ARP_CACHE_SIZE;
    for (; entry != end; ++entry)
    {
        if (entry->pa.n[0] == pa->n[0] &&
            entry->pa.n[1] == pa->n[1] &&
            entry->pa.n[2] == pa->n[2] &&
            entry->pa.n[3] == pa->n[3])
        {
            return entry;
        }
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
const Eth_Addr* arp_lookup_mac(const IPv4_Addr* pa)
{
    ARP_Entry* entry = arp_lookup(pa);
    if (entry)
    {
        return &entry->ha;
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
void arp_rx(const u8* pkt, uint len)
{
    arp_print(pkt, len);

    // Decode Base Header
    if (len < 8)
    {
        return;
    }

    u16 htype = (pkt[0] << 8) | pkt[1];
    u16 ptype = (pkt[2] << 8) | pkt[3];
    u16 op = (pkt[6] << 8) | pkt[7];

    // Skip packets that are not Ethernet, IPv4, or well-formed
    if (htype != ARP_HTYPE_ETH || ptype != ET_IPV4 || len < 28)
    {
        return;
    }

    // Decode addresses
    const Eth_Addr* sha = (const Eth_Addr*)(pkt + 8);
    const IPv4_Addr* spa = (const IPv4_Addr*)(pkt + 14);
    //const Eth_Addr* tha = (const Eth_Addr*)(pkt + 18);
    const IPv4_Addr* tpa = (const IPv4_Addr*)(pkt + 24);

    // Update existing entry if we know about this source IP address
    bool merge = false;
    ARP_Entry* entry = arp_lookup(spa);
    if (entry)
    {
        if (net_trace)
        {
            console_print("ARP Merging entry\n");
        }

        entry->ha = *sha;
        merge = true;
    }

    // Check if this ARP packet is targeting our IP
    if (tpa->n[0] == net_local_ip.n[0] &&
        tpa->n[1] == net_local_ip.n[1] &&
        tpa->n[2] == net_local_ip.n[2] &&
        tpa->n[3] == net_local_ip.n[3])
    {
        // Add a new entry if we didn't update earlier.
        if (!merge)
        {
            if (net_trace)
            {
                console_print("ARP Adding entry\n");
            }

            arp_add(sha, spa);
        }

        // Respond to requests.
        if (op == ARP_OP_REQUEST)
        {
            arp_reply(sha, spa);
        }
    }
}
