/**
 * @file     netLookup.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <raw.h>

/**
 * @ingroup network
 *
 * デバイスからネットワークインタフェースを検索する.
 * @param devnum ネットワークインタフェースを検索するデバイス番号
 * @return ネットワークインタフェース, 存在しなかったら NULL
 */
struct netif *netLookup(int devnum)
{
#if NNETIF
    int i;

    for (i = 0; i < NNETIF; i++)
    {
        /* Check if network interface is allocated and device matches */
        if ((NET_ALLOC == netiftab[i].state)
            && (netiftab[i].dev == devnum))
        {
            return &netiftab[i];
        }
    }
#endif
    return NULL;
}
