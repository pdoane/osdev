// ------------------------------------------------------------------------------------------------
// net/checksum.c
// ------------------------------------------------------------------------------------------------

#include "net/checksum.h"

// ------------------------------------------------------------------------------------------------
u16 NetChecksum(const u8 *data, const u8 *end)
{
    uint sum = NetChecksumAcc(data, end, 0);
    return NetChecksumFinal(sum);
}

// ------------------------------------------------------------------------------------------------
uint NetChecksumAcc(const u8 *data, const u8 *end, uint sum)
{
    uint len = end - data;
    u16 *p = (u16 *)data;

    while (len > 1)
    {
        sum += *p++;
        len -= 2;
    }

    if (len)
    {
        sum += *(u8 *)p;
    }

    return sum;
}

// ------------------------------------------------------------------------------------------------
u16 NetChecksumFinal(uint sum)
{
    sum = (sum & 0xffff) + (sum >> 16);
    sum += (sum >> 16);

    u16 temp = ~sum;
    return ((temp & 0x00ff) << 8) | ((temp & 0xff00) >> 8); // TODO - shouldn't swap this twice
}
