// ------------------------------------------------------------------------------------------------
// net/dhcp.c
// ------------------------------------------------------------------------------------------------

#include "net/dhcp.h"
#include "net/buf.h"
#include "net/dns.h"
#include "net/ipv4.h"
#include "net/net.h"
#include "net/port.h"
#include "net/route.h"
#include "net/swap.h"
#include "net/udp.h"
#include "console/console.h"
#include "stdlib/string.h"

// ------------------------------------------------------------------------------------------------
// DHCP Header

typedef struct DhcpHeader
{
    u8 opcode;
    u8 htype;
    u8 hlen;
    u8 hopCount;
    u32 xid;
    u16 secCount;
    u16 flags;
    Ipv4Addr clientIpAddr;
    Ipv4Addr yourIpAddr;
    Ipv4Addr serverIpAddr;
    Ipv4Addr gatewayIpAddr;
    EthAddr clientEthAddr;
    u8 reserved[10];
    char serverName[64];
    char bootFilename[128];
} PACKED DhcpHeader;

// ------------------------------------------------------------------------------------------------
// Opcode

#define OP_REQUEST                      1
#define OP_REPLY                        2

// ------------------------------------------------------------------------------------------------
// Hardware Address Type

#define HTYPE_ETH                       1

// ------------------------------------------------------------------------------------------------
// Options

#define MAGIC_COOKIE                    0x63825363

#define OPT_PAD                         0
#define OPT_SUBNET_MASK                 1
#define OPT_ROUTER                      3
#define OPT_DNS                         6
#define OPT_REQUESTED_IP_ADDR           50
#define OPT_LEASE_TIME                  51
#define OPT_DHCP_MESSAGE_TYPE           53
#define OPT_SERVER_ID                   54
#define OPT_PARAMETER_REQUEST           55
#define OPT_END                         255

typedef struct DhcpOptions
{
    const Ipv4Addr *subnetMask;
    const Ipv4Addr *routerList;
    const Ipv4Addr *routerEnd;
    const Ipv4Addr *dnsList;
    const Ipv4Addr *dnsEnd;
    const Ipv4Addr *requestedIpAddr;
    uint leaseTime;
    uint messageType;
    const Ipv4Addr *serverId;
    const u8 *parameterList;
    const u8 *parameterEnd;
} DhcpOptions;

// ------------------------------------------------------------------------------------------------
// Message Types

#define DHCP_DISCOVER                   1
#define DHCP_OFFER                      2
#define DHCP_REQUEST                    3
#define DHCP_DECLINE                    4
#define DHCP_ACK                        5
#define DHCP_NAK                        6
#define DHCP_RELEASE                    7
#define DHCP_INFORM                     8

// ------------------------------------------------------------------------------------------------
static bool DhcpParseOptions(DhcpOptions *opt, const NetBuf *pkt)
{
    const u8 *p = pkt->start + sizeof(DhcpHeader);
    const u8 *end = pkt->end;

    if (p + 4 > end)
    {
        return false;
    }

    u32 magicCookie = NetSwap32(*(u32 *)p);
    p += 4;

    if (magicCookie != MAGIC_COOKIE)
    {
        return false;
    }

    memset(opt, 0, sizeof(*opt));

    while (p < end)
    {
        u8 type = *p++;

        if (type == OPT_PAD)
        {
            continue;
        }
        else if (type == OPT_END)
        {
            break;
        }
        else
        {
            u8 optLen = *p++;

            const u8 *next = p + optLen;
            if (next > end)
            {
                return false;
            }

            switch (type)
            {
            case OPT_SUBNET_MASK:
                opt->subnetMask = (const Ipv4Addr *)p;
                break;
            case OPT_ROUTER:
                opt->routerList = (const Ipv4Addr *)p;
                opt->routerEnd = (const Ipv4Addr *)next;
                break;
            case OPT_DNS:
                opt->dnsList = (const Ipv4Addr *)p;
                opt->dnsEnd = (const Ipv4Addr *)next;
                break;
            case OPT_REQUESTED_IP_ADDR:
                opt->requestedIpAddr = (const Ipv4Addr *)p;
                break;
            case OPT_LEASE_TIME:
                opt->leaseTime = NetSwap32(*(u32 *)p);
                break;
            case OPT_DHCP_MESSAGE_TYPE:
                opt->messageType = *p;
                break;
            case OPT_SERVER_ID:
                opt->serverId = (const Ipv4Addr *)p;
                break;
            case OPT_PARAMETER_REQUEST:
                opt->parameterList = p;
                opt->parameterEnd = next;
                break;
            default:
                ConsolePrint("   DHCP: unknown option (%d)\n", type);
                break;
            }

            p = next;
        }
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
static u8 *DhcpBuildHeader(NetBuf *pkt, uint xid, const EthAddr *clientEthAddr, u8 messageType)
{
    DhcpHeader *hdr = (DhcpHeader *)pkt->start;

    memset(hdr, 0, sizeof(DhcpHeader));
    hdr->opcode = OP_REQUEST;
    hdr->htype = HTYPE_ETH;
    hdr->hlen = sizeof(EthAddr);
    hdr->hopCount = 0;
    hdr->xid = NetSwap32(xid);
    hdr->secCount = NetSwap16(0);
    hdr->flags = NetSwap16(0);
    hdr->clientEthAddr = *clientEthAddr;

    // Options
    u8 *p = pkt->start + sizeof(DhcpHeader);

    // Magic Cookie
    *(u32 *)p = NetSwap32(MAGIC_COOKIE);
    p += 4;

    // DHCP Message Type
    *p++ = OPT_DHCP_MESSAGE_TYPE;
    *p++ = 1;
    *p++ = messageType;

    return p;
}

// ------------------------------------------------------------------------------------------------
static void DhcpRequest(NetIntf *intf, const DhcpHeader *hdr, const DhcpOptions *opt)
{
    uint xid = NetSwap32(hdr->xid);
    const Ipv4Addr *requestedIpAddr = &hdr->yourIpAddr;
    const Ipv4Addr *serverId = opt->serverId;

    char requestedIpAddrStr[IPV4_ADDR_STRING_SIZE];
    Ipv4AddrToStr(requestedIpAddrStr, sizeof(requestedIpAddrStr), requestedIpAddr);
    ConsolePrint("DHCP requesting lease for %s\n", requestedIpAddrStr);

    NetBuf *pkt = NetAllocBuf();

    // Header
    u8 *p = DhcpBuildHeader(pkt, xid, &intf->ethAddr, DHCP_REQUEST);

    // Server Identifier
    *p++ = OPT_SERVER_ID;
    *p++ = sizeof(Ipv4Addr);
    *(Ipv4Addr *)p = *serverId;
    p += sizeof(Ipv4Addr);

    // Requested IP address
    *p++ = OPT_REQUESTED_IP_ADDR;
    *p++ = sizeof(Ipv4Addr);
    *(Ipv4Addr *)p = *requestedIpAddr;
    p += sizeof(Ipv4Addr);

    // Parameter Request list
    *p++ = OPT_PARAMETER_REQUEST;
    *p++ = 3;
    *p++ = OPT_SUBNET_MASK;
    *p++ = OPT_ROUTER;
    *p++ = OPT_DNS;

    // Option End
    *p++ = OPT_END;

    // Send packet
    pkt->end = p;

    DhcpPrint(pkt);
    UdpSendIntf(intf, &g_broadcastIpv4Addr, PORT_BOOTP_SERVER, PORT_BOOTP_CLIENT, pkt);
}


// ------------------------------------------------------------------------------------------------
static void DhcpAck(NetIntf *intf, const DhcpHeader *hdr, const DhcpOptions *opt)
{
    // Update interface IP address
    intf->ipAddr = hdr->yourIpAddr;

    // Add gateway route
    if (opt->routerList)
    {
        NetAddRoute(&g_nullIpv4Addr, &g_nullIpv4Addr, opt->routerList, intf);
    }

    // Add subnet route
    if (opt->subnetMask)
    {
        Ipv4Addr subnetAddr;
        subnetAddr.u.bits = intf->ipAddr.u.bits & opt->subnetMask->u.bits;
        NetAddRoute(&subnetAddr, opt->subnetMask, 0, intf);
    }

    // Add host route
    Ipv4Addr hostMask = { { { 0xff, 0xff, 0xff, 0xff } } };
    NetAddRoute(&intf->ipAddr, &hostMask, 0, intf);

    // Record broadcast address
    if (opt->subnetMask)
    {
        intf->broadcastAddr.u.bits = intf->ipAddr.u.bits | ~opt->subnetMask->u.bits;
    }

    // Set DNS server
    if (opt->dnsList)
    {
        g_dnsServer = *opt->dnsList;
    }

    // TODO - how to handle ARP with DHCP sequence?
}

// ------------------------------------------------------------------------------------------------
void DhcpRecv(NetIntf *intf, const NetBuf *pkt)
{
    DhcpPrint(pkt);

    if (pkt->start + sizeof(DhcpHeader) > pkt->end)
    {
        return;
    }

    const DhcpHeader *hdr = (const DhcpHeader *)pkt->start;
    if (hdr->opcode != OP_REPLY || hdr->htype != HTYPE_ETH || hdr->hlen != sizeof(EthAddr))
    {
        return;
    }

    if (!EthAddrEq(&intf->ethAddr, &hdr->clientEthAddr))
    {
        return;
    }

    DhcpOptions opt;
    if (!DhcpParseOptions(&opt, pkt))
    {
        return;
    }

    char yourIpAddrStr[IPV4_ADDR_STRING_SIZE];
    Ipv4AddrToStr(yourIpAddrStr, sizeof(yourIpAddrStr), &hdr->yourIpAddr);

    switch (opt.messageType)
    {
    case DHCP_OFFER:
        ConsolePrint("DHCP offer received for %s\n", yourIpAddrStr);
        DhcpRequest(intf, hdr, &opt);
        break;

    case DHCP_ACK:
        ConsolePrint("DHCP ack received for %s\n", yourIpAddrStr);
        DhcpAck(intf, hdr, &opt);
        break;

    case DHCP_NAK:
        ConsolePrint("DHCP nak received for %s\n", yourIpAddrStr);
        break;

    default:
        ConsolePrint("DHCP message unhandled\n");
        break;
    }
}

// ------------------------------------------------------------------------------------------------
void DhcpDiscover(NetIntf *intf)
{
    ConsolePrint("DHCP discovery\n");

    NetBuf *pkt = NetAllocBuf();

    // Header
    uint xid = 0;
    u8 *p = DhcpBuildHeader(pkt, xid, &intf->ethAddr, DHCP_DISCOVER);

    // Parameter Request list
    *p++ = OPT_PARAMETER_REQUEST;
    *p++ = 3;
    *p++ = OPT_SUBNET_MASK;
    *p++ = OPT_ROUTER;
    *p++ = OPT_DNS;

    // Option End
    *p++ = OPT_END;

    // Send packet
    pkt->end = p;

    DhcpPrint(pkt);
    UdpSendIntf(intf, &g_broadcastIpv4Addr, PORT_BOOTP_SERVER, PORT_BOOTP_CLIENT, pkt);
}

// ------------------------------------------------------------------------------------------------
void DhcpPrint(const NetBuf *pkt)
{
    if (~g_netTrace & TRACE_APP)
    {
        return;
    }

    if (pkt->start + sizeof(DhcpHeader) > pkt->end)
    {
        return;
    }

    const DhcpHeader *hdr = (const DhcpHeader *)pkt->start;

    char clientIpAddrStr[IPV4_ADDR_STRING_SIZE];
    char yourIpAddrStr[IPV4_ADDR_STRING_SIZE];
    char serverIpAddrStr[IPV4_ADDR_STRING_SIZE];
    char gatewayIpAddrStr[IPV4_ADDR_STRING_SIZE];
    char clientEthAddrStr[ETH_ADDR_STRING_SIZE];

    Ipv4AddrToStr(clientIpAddrStr, sizeof(clientIpAddrStr), &hdr->clientIpAddr);
    Ipv4AddrToStr(yourIpAddrStr, sizeof(yourIpAddrStr), &hdr->yourIpAddr);
    Ipv4AddrToStr(serverIpAddrStr, sizeof(serverIpAddrStr), &hdr->serverIpAddr);
    Ipv4AddrToStr(gatewayIpAddrStr, sizeof(gatewayIpAddrStr), &hdr->gatewayIpAddr);
    EthAddrToStr(clientEthAddrStr, sizeof(clientEthAddrStr), &hdr->clientEthAddr);

    ConsolePrint("   DHCP: opcode=%d htype=%d hlen=%d hopCount=%d xid=%d secs=%d flags=%d len=%d\n",
        hdr->opcode, hdr->htype, hdr->hlen, hdr->hopCount,
        NetSwap32(hdr->xid), NetSwap16(hdr->secCount), NetSwap16(hdr->flags), pkt->end - pkt->start);
    ConsolePrint("   DHCP: client=%s your=%s server=%s gateway=%s\n",
        clientIpAddrStr, yourIpAddrStr, serverIpAddrStr, gatewayIpAddrStr);
    ConsolePrint("   DHCP: eth=%s serverName=%s bootFilename=%s\n",
        clientEthAddrStr, hdr->serverName, hdr->bootFilename);

    DhcpOptions opt;
    if (!DhcpParseOptions(&opt, pkt))
    {
        return;
    }

    char ipv4AddrStr[IPV4_ADDR_STRING_SIZE];

    if (opt.messageType)
    {
        ConsolePrint("   DHCP: message type: %d\n", opt.messageType);
    }

    if (opt.subnetMask)
    {
        Ipv4AddrToStr(ipv4AddrStr, sizeof(ipv4AddrStr), opt.subnetMask);
        ConsolePrint("   DHCP: subnetMask: %s\n", ipv4AddrStr);
    }

    for (const Ipv4Addr *addr = opt.routerList; addr != opt.routerEnd; ++addr)
    {
        Ipv4AddrToStr(ipv4AddrStr, sizeof(ipv4AddrStr), addr);
        ConsolePrint("   DHCP: router: %s\n", ipv4AddrStr);
    }

    for (const Ipv4Addr *addr = opt.dnsList; addr != opt.dnsEnd; ++addr)
    {
        Ipv4AddrToStr(ipv4AddrStr, sizeof(ipv4AddrStr), addr);
        ConsolePrint("   DHCP: dns: %s\n", ipv4AddrStr);
    }

    if (opt.requestedIpAddr)
    {
        Ipv4AddrToStr(ipv4AddrStr, sizeof(ipv4AddrStr), opt.requestedIpAddr);
        ConsolePrint("   DHCP: requested ip: %s\n", ipv4AddrStr);
    }

    if (opt.leaseTime)
    {
        ConsolePrint("   DHCP: lease time: %d\n", opt.leaseTime);
    }

    if (opt.serverId)
    {
        Ipv4AddrToStr(ipv4AddrStr, sizeof(ipv4AddrStr), opt.serverId);
        ConsolePrint("   DHCP: server id: %s\n", ipv4AddrStr);
    }

    for (const u8 *p = opt.parameterList; p != opt.parameterEnd; ++p)
    {
        ConsolePrint("   DHCP: parameter request: %d\n", *p);
    }
}
