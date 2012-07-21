// ------------------------------------------------------------------------------------------------
// net/buf.c
// ------------------------------------------------------------------------------------------------

#include "net/buf.h"
#include "mem/vm.h"

// ------------------------------------------------------------------------------------------------
static Net_Buf* net_free_bufs;
int net_buf_alloc_count;

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
    buf->start = (u8*)buf + NET_BUF_START;
    buf->end = (u8*)buf + NET_BUF_START;
    buf->ref_count = 1;

    ++net_buf_alloc_count;
    return buf;
}

// ------------------------------------------------------------------------------------------------
void net_release_buf(Net_Buf* buf)
{
    if (!--buf->ref_count)
    {
        --net_buf_alloc_count;

        buf->next_buf = net_free_bufs;
        net_free_bufs = buf;
    }
}
