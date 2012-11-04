// ------------------------------------------------------------------------------------------------
// net/buf.c
// ------------------------------------------------------------------------------------------------

#include "net/buf.h"
#include "mem/vm.h"

// ------------------------------------------------------------------------------------------------
static Link s_netFreeBufs = { &s_netFreeBufs, &s_netFreeBufs };
int g_netBufAllocCount;

// ------------------------------------------------------------------------------------------------
NetBuf *NetAllocBuf()
{
    NetBuf *buf;

    if (ListIsEmpty(&s_netFreeBufs))
    {
        buf = VMAlloc(NET_BUF_SIZE);
    }
    else
    {
        buf = LinkData(s_netFreeBufs.next, NetBuf, link);
        LinkRemove(&buf->link);
    }

    buf->link.prev = 0;
    buf->link.next = 0;
    buf->start = (u8 *)buf + NET_BUF_START;
    buf->end = (u8 *)buf + NET_BUF_START;
    buf->refCount = 1;

    ++g_netBufAllocCount;
    return buf;
}

// ------------------------------------------------------------------------------------------------
void NetReleaseBuf(NetBuf *buf)
{
    if (!--buf->refCount)
    {
        --g_netBufAllocCount;

        LinkAfter(&s_netFreeBufs, &buf->link);
    }
}
