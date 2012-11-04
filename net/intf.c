// ------------------------------------------------------------------------------------------------
// net/intf.c
// ------------------------------------------------------------------------------------------------

#include "net/intf.h"
#include "mem/vm.h"
#include "stdlib/string.h"

// ------------------------------------------------------------------------------------------------
// Globals

Link g_netIntfList = { &g_netIntfList, &g_netIntfList };

// ------------------------------------------------------------------------------------------------
NetIntf *NetIntfCreate()
{
    NetIntf *intf = VMAlloc(sizeof(NetIntf));
    memset(intf, 0, sizeof(NetIntf));
    LinkInit(&intf->link);

    return intf;
}

// ------------------------------------------------------------------------------------------------
void NetIntfAdd(NetIntf *intf)
{
    LinkBefore(&g_netIntfList, &intf->link);
}
