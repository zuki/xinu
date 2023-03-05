/**
 * @file rtClear.c
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
 * ルーティングテーブルから指定されたネットワークインタフェースの
 * すべてのエントリを削除する.
 * @param nif ネットワークインタフェース
 * @return エントリの削除に成功した場合は OK; そうでなければ SYSERR
 */
syscall rtClear(struct netif *nif)
{
    int i;
    irqmask im;

    /* 1. 引数のエラーチェック */
    if (NULL == nif)
    {
        return SYSERR;
    }

    im = disable();
    for (i = 0; i < RT_NENTRY; i++)
    {
        if ((RT_USED == rttab[i].state) && (nif == rttab[i].nif))
        {
            rttab[i].state = RT_FREE;
            rttab[i].nif = NULL;
        }
    }
    restore(im);
    return OK;
}
