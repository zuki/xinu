/**
 * @file ipv4RecvDemux.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <ipv4.h>
#include <network.h>

/**
 *  @ingroup ipv4
 *
 *  IPパケットがこのネットワークインタフェース向けのものであるか決定する.
 *  @param dst 宛先IPアドレス
 *  @return このネットワークインタフェース向けの場合は TRUE;
 * そうでなければ FLASE
 */
bool ipv4RecvDemux(struct netaddr *dst)
{
#if NNETIF
    int i;
    struct netif *netptr;

    /*  すべてのインタフェースについてチェックする */
    for (i = 0; i < NNETIF; i++)
    {
        netptr = &netiftab[i];

        /* 未使用のインタフェースはスキップ */
        if (netptr->state != NET_ALLOC)
        {
            continue;
        }

        /* 宛先IPアドレスがインタフェースのIPアドレスに一致するか
         * チェックする */
        if (netaddrequal(dst, &netptr->ip)
            || netaddrequal(dst, &netptr->ipbrc))
        {
            return TRUE;
        }
    }
#endif                          /* NNETIF */
    return FALSE;
}
