/**
 * @file rtLookup.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <interrupt.h>
#include <network.h>
#include <route.h>

/**
 * @ingroup route
 *
 * ルーティングテーブルを検索する
 * @param addr ルーティングが必要なIPアドレス
 * @return ルートテーブルエントリ; マッチするエントリがなかった場合はNULL;
 *         エラーが発生した場合は SYSERR
 */
struct rtEntry *rtLookup(const struct netaddr *addr)
{
    int i;
    struct rtEntry *rtptr;
    struct netaddr masked;
    irqmask im;

    rtptr = NULL;

    RT_TRACE("Addr = %d.%d.%d.%d", addr->addr[0], addr->addr[1],
             addr->addr[2], addr->addr[3]);

    im = disable();
    for (i = 0; i < RT_NENTRY; i++)
    {
        if (RT_USED == rttab[i].state)
        {
            /* 1. アドレスにマスクを適用 */
            netaddrcpy(&masked, addr);
            netaddrmask(&masked, &rttab[i].mask);

            /* 2. マッチするかチェック  */
            if (netaddrequal(&masked, &rttab[i].dst))
            {
                RT_TRACE("Matched entry %d", i);
                /* これまでにマッチしたものがなかった、あるいはより良いものに
                   マッチした場合は記憶する */
                if ((NULL == rtptr)
                    || (rtptr->masklen < rttab[i].masklen))
                {
                    rtptr = &rttab[i];
                }
            }
        }
    }
    restore(im);

    return rtptr;
}
