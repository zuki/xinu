/**
 * @file rtAlloc.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <interrupt.h>
#include <route.h>

/**
 * @ingroup route
 *
 * ルートテーブルからエントリを割り当てる.
 * @return ルートテーブルのエントリ; 全て四王済みの場合はNULL;
 *         エラーが発生したら SYSERR
 */
struct rtEntry *rtAlloc(void)
{
    int i;
    irqmask im;

    RT_TRACE("Allocating route entry");

    im = disable();
    for (i = 0; i < RT_NENTRY; i++)
    {
        /* 未使用のエントリだったら、そのエントリを返す */
        if (RT_FREE == rttab[i].state)
        {
            rttab[i].state = RT_PEND;
            RT_TRACE("Free entry %d", i);
            restore(im);
            return &rttab[i];
        }
    }

    restore(im);
    RT_TRACE("No free entry");
    return NULL;
}
