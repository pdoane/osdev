// ------------------------------------------------------------------------------------------------
// net/buf.c
// ------------------------------------------------------------------------------------------------

#include "net/buf.h"
#include "mem/vm.h"

// ------------------------------------------------------------------------------------------------
static Link net_free_bufs = { &net_free_bufs, &net_free_bufs };
int net_buf_alloc_count;

// ------------------------------------------------------------------------------------------------
Net_Buf* net_alloc_buf()
{
    Net_Buf* buf;

    if (list_empty(&net_free_bufs))
    {
        buf = vm_alloc(NET_BUF_SIZE);
    }
    else
    {
        buf = link_data(net_free_bufs.next, Net_Buf, link);
        link_remove(&buf->link);
    }

    buf->link.prev = 0;
    buf->link.next = 0;
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

        link_after(&net_free_bufs, &buf->link);
    }
}
