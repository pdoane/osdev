// ------------------------------------------------------------------------------------------------
// arp.c
// ------------------------------------------------------------------------------------------------

#include "arp.h"
#include "console.h"
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
    u8 ha[6];
    u8 pa[4];
} ARP_Entry;

static ARP_Entry arp_cache[ARP_CACHE_SIZE];

// ------------------------------------------------------------------------------------------------
static void arp_print(u8* pkt, uint len)
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
        u8* sha = pkt + 8;
        u8* spa = pkt + 14;
        u8* tha = pkt + 18;
        u8* tpa = pkt + 24;

        console_print(" ARP: sha=%02x:%02x:%02x:%02x:%02x:%02x spa=%d.%d.%d.%d\n",
                sha[0], sha[1], sha[2], sha[3], sha[4], sha[5],
                spa[0], spa[1], spa[2], spa[3]);
        console_print(" ARP: tha=%02x:%02x:%02x:%02x:%02x:%02x tpa=%d.%d.%d.%d\n",
                tha[0], tha[1], tha[2], tha[3], tha[4], tha[5],
                tpa[0], tpa[1], tpa[2], tpa[3]);
    }
}

// ------------------------------------------------------------------------------------------------
static void arp_snd(uint op, u8* tha, u8* tpa)
{
    u8 data[256];

    u8* arp_pkt = net_eth_hdr(data, tha, ET_ARP);

    // HTYPE
    arp_pkt[0] = (ARP_HTYPE_ETH >> 8) & 0xff;
    arp_pkt[1] = (ARP_HTYPE_ETH) & 0xff;

    // PTYPE
    arp_pkt[2] = (ET_IPV4 >> 8) & 0xff;
    arp_pkt[3] = (ET_IPV4) & 0xff;

    // HLEN
    arp_pkt[4] = 6;

    // PLEN
    arp_pkt[5] = 4;

    // Operation
    arp_pkt[6] = (op >> 8) & 0xff;
    arp_pkt[7] = (op) & 0xff;

    // SHA
    memcpy(arp_pkt + 8, net_local_mac, 6);

    // SPA
    arp_pkt[14] = net_local_ip[0];
    arp_pkt[15] = net_local_ip[1];
    arp_pkt[16] = net_local_ip[2];
    arp_pkt[17] = net_local_ip[3];

    // THA
    if (op == ARP_OP_REQUEST)
    {
        memset(arp_pkt + 18, 0, 6);
    }
    else
    {
        memcpy(arp_pkt + 18, tha, 6);
    }

    // TPA
    arp_pkt[24] = tpa[0];
    arp_pkt[25] = tpa[1];
    arp_pkt[26] = tpa[2];
    arp_pkt[27] = tpa[3];

    arp_print(arp_pkt, 28);

    uint len = 14 + 28;

    // Transmit packet
    net_tx(data, len);
}

// ------------------------------------------------------------------------------------------------
void arp_request(u8* tpa)
{
    arp_snd(ARP_OP_REQUEST, net_broadcast_mac, tpa);
}

// ------------------------------------------------------------------------------------------------
void arp_reply(u8* tha, u8* tpa)
{
    arp_snd(ARP_OP_REPLY, tha, tpa);
}

// ------------------------------------------------------------------------------------------------
static void arp_add(u8* ha, u8* pa)
{
    // TODO - handle overflow
    ARP_Entry* entry = arp_cache;
    ARP_Entry* end = entry + ARP_CACHE_SIZE;
    for (; entry != end; ++entry)
    {
        if ((entry->pa[0] == 0 &&
             entry->pa[1] == 0 &&
             entry->pa[2] == 0 &&
             entry->pa[3] == 0))
        {
            break;
        }
    }

    if (entry != end)
    {
        memcpy(entry->ha, ha, 6);
        memcpy(entry->pa, pa, 4);
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
        memset(entry->ha, 0xff, 6);
        memset(entry->pa, 0, 4);
    }
}

// ------------------------------------------------------------------------------------------------
static ARP_Entry* arp_lookup(u8* pa)
{
    ARP_Entry* entry = arp_cache;
    ARP_Entry* end = entry + ARP_CACHE_SIZE;
    for (; entry != end; ++entry)
    {
        if (entry->pa[0] == pa[0] &&
            entry->pa[1] == pa[1] &&
            entry->pa[2] == pa[2] &&
            entry->pa[3] == pa[3])
        {
            return entry;
        }
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
u8* arp_lookup_mac(u8* pa)
{
    ARP_Entry* entry = arp_lookup(pa);
    if (entry)
    {
        return entry->ha;
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
void arp_rx(u8* pkt, uint len)
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
    u8* sha = pkt + 8;
    u8* spa = pkt + 14;
    //u8* tha = pkt + 18;
    u8* tpa = pkt + 24;

    // Update existing entry if we know about this source IP address
    bool merge = false;
    ARP_Entry* entry = arp_lookup(spa);
    if (entry)
    {
        if (net_trace)
        {
            console_print("ARP Merging entry\n");
        }

        memcpy(entry->ha, sha, 6);
        merge = true;
    }

    // Check if this ARP packet is targeting our IP
    if (tpa[0] == net_local_ip[0] &&
        tpa[1] == net_local_ip[1] &&
        tpa[2] == net_local_ip[2] &&
        tpa[3] == net_local_ip[3])
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
