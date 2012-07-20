// ------------------------------------------------------------------------------------------------
// net/buf.c
// ------------------------------------------------------------------------------------------------

#include "net/buf.h"
#include "mem/vm.h"

// ------------------------------------------------------------------------------------------------
static Net_Buf* net_free_bufs;

// ------------------------------------------------------------------------------------------------
Net_Buf* net_alloc_buf()
{
    Net_Buf* buf = net_free_bufs;
    if (!buf)
    {
        buf = vm_alloc(NET_BUF_SIZE);
    }
    else
    {
        net_free_bufs = buf->next_buf;
    }

    buf->next_buf = 0;
    buf->next_pkt = 0;
    buf->start = (u8*)buf + 256;   // room for Net_Buf header + eth header + ip header + tcp header
    buf->end = (u8*)buf + 256;

    return buf;
}

// ------------------------------------------------------------------------------------------------
void net_free_buf(Net_Buf* buf)
{
    buf->next_buf = net_free_bufs;
    net_free_bufs = buf;
}
