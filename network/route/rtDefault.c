/**
 * @file rtDefault.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <network.h>
#include <route.h>
#include <stdlib.h>

/**
 * @ingroup route
 *
 * デフォルトルートを設定する.
 * @param gate デフォルトルートのゲートウェイ
 * @param nif デフォルトルートのネットワークインタフェース
 * @return 追加/更新に成功した場合は OK; そうでなければ SYSERR
 */
syscall rtDefault(const struct netaddr *gate, struct netif *nif)
{
    struct rtEntry *rtptr;
    int i;
    struct netaddr mask;

    /* 1. 引数のエラーチェック */
    if ((NULL == gate) || (NULL == nif) || (gate->len > NET_MAX_ALEN))
    {
        RT_TRACE("Invalid args");
        return SYSERR;
    }

    /* 2. デフォルトルートのマスクを設定 */
    mask.type = gate->type;
    mask.len = gate->len;
    bzero(mask.addr, mask.len);

    /* 3. デフォルトルートがすでに存在していないかチェック */
    rtptr = NULL;
    for (i = 0; i < RT_NENTRY; i++)
    {
        if ((RT_USED == rttab[i].state)
            && (netaddrequal(&rttab[i].mask, &mask)))
        {
            RT_TRACE("Default route exists, entry %d", i);
            return OK;
        }
    }

    /* 4. ルートテーブルエントリを割り当てる */
    rtptr = rtAlloc();
    if ((SYSERR == (int)rtptr) || (NULL == rtptr))
    {
        RT_TRACE("Failed to allocate route entry");
        return SYSERR;
    }

    /* 5. エントリの情報をセットする */
    netaddrcpy(&rtptr->dst, &mask);
    netaddrcpy(&rtptr->gateway, gate);
    netaddrcpy(&rtptr->mask, &mask);
    rtptr->nif = nif;

    /* 6. マスク長をセットする */
    rtptr->masklen = 0;

    rtptr->state = RT_USED;
    RT_TRACE("Populated default route");
    return OK;
}
