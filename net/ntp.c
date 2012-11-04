// ------------------------------------------------------------------------------------------------
// net/ntp.c
// ------------------------------------------------------------------------------------------------

#include "net/ntp.h"
#include "net/buf.h"
#include "net/net.h"
#include "net/port.h"
#include "net/swap.h"
#include "net/udp.h"
#include "console/console.h"
#include "time/rtc.h"
#include "time/time.h"

// ------------------------------------------------------------------------------------------------
// NTP Constants

#define NTP_VERSION                     4
#define UNIX_EPOCH                      0x83aa7e80

// ------------------------------------------------------------------------------------------------
// NTP Packet

typedef struct NtpHeader
{
    u8 mode;
    u8 stratum;
    u8 poll;
    i8 precision;
    u32 rootDelay;
    u32 rootDispersion;
    u32 refId;
    u64 refTimestamp;
    u64 origTimestamp;
    u64 recvTimestamp;
    u64 sendTimestamp;
} PACKED NtpHeader;

// ------------------------------------------------------------------------------------------------
// NTP Modes

#define MODE_CLIENT                     3
#define MODE_SERVER                     4

// ------------------------------------------------------------------------------------------------
void NtpRecv(NetIntf *intf, const NetBuf *pkt)
{
    NtpPrint(pkt);

    if (pkt->start + sizeof(NtpHeader) > pkt->end)
    {
        return;
    }

    NtpHeader *hdr = (NtpHeader *)pkt->start;

    abs_time t = (NetSwap64(hdr->sendTimestamp) >> 32) - UNIX_EPOCH;
    DateTime dt;
    SplitTime(&dt, t, g_localTimeZone);

    char str[TIME_STRING_SIZE];
    FormatTime(str, sizeof(str), &dt);
    ConsolePrint("Setting time to %s\n", str);

    RtcSetTime(&dt);
}

// ------------------------------------------------------------------------------------------------
void NtpSend(const Ipv4Addr *dstAddr)
{
    NetBuf *pkt = NetAllocBuf();

    NtpHeader *hdr = (NtpHeader *)pkt->start;
    hdr->mode = (NTP_VERSION << 3) | MODE_CLIENT;
    hdr->stratum = 0;
    hdr->poll = 4;
    hdr->precision = -6;
    hdr->rootDelay = NetSwap32(1 << 16);
    hdr->rootDispersion = NetSwap32(1 << 16);
    hdr->refId = NetSwap32(0);
    hdr->refTimestamp = NetSwap64(0);
    hdr->origTimestamp = NetSwap64(0);
    hdr->recvTimestamp = NetSwap64(0);
    hdr->sendTimestamp = NetSwap64(0);

    pkt->end += sizeof(NtpHeader);
    uint srcPort = NetEphemeralPort();

    NtpPrint(pkt);
    UdpSend(dstAddr, PORT_NTP, srcPort, pkt);
}

// ------------------------------------------------------------------------------------------------
void NtpPrint(const NetBuf *pkt)
{
    if (~g_netTrace & TRACE_APP)
    {
        return;
    }

    if (pkt->start + sizeof(NtpHeader) > pkt->end)
    {
        return;
    }

    NtpHeader *hdr = (NtpHeader *)pkt->start;

    abs_time t = (NetSwap64(hdr->sendTimestamp) >> 32) - UNIX_EPOCH;

    ConsolePrint("   NTP: mode=%02x stratum=%d poll=%d precision=%d\n",
        hdr->mode, hdr->stratum, hdr->poll, hdr->precision);
    ConsolePrint("   NTP: rootDelay=%08x rootDispersion=%08x refId=%08x\n",
        NetSwap32(hdr->rootDelay), NetSwap32(hdr->rootDispersion), NetSwap32(hdr->refId));
    ConsolePrint("   NTP: refTimestamp=%016x origTimestamp=%016x\n",
        NetSwap64(hdr->refTimestamp), NetSwap64(hdr->origTimestamp));
    ConsolePrint("   NTP: recvTimestamp=%016x sendTimestamp=%016x\n",
        NetSwap64(hdr->recvTimestamp), NetSwap64(hdr->sendTimestamp));
    ConsolePrint("   NTP: unix_epoch=%u\n",
        t);
}

