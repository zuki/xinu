/**
 * @file rtRemove.c
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
 * 宛先に基づいてエントリを削除する.
 * @param dst 宛先IPアドレス
 * @return エントリの削除に成功した場合は OK; そうでなければ SYSERR
 */
syscall rtRemove(const struct netaddr *dst)
{
    int i;
    irqmask im;

    /* 1. 引数のエラーチェック */
    if (NULL == dst)
    {
        return SYSERR;
    }

    im = disable();
    for (i = 0; i < RT_NENTRY; i++)
    {
        if ((RT_USED == rttab[i].state)
            && netaddrequal(dst, &rttab[i].dst))
        {
            rttab[i].state = RT_FREE;
            rttab[i].nif = NULL;
        }
    }
    restore(im);
    return OK;
}
