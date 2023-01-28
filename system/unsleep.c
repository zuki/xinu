/**
 * @file unsleep.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <kernel.h>
#include <stddef.h>
#include <interrupt.h>
#include <thread.h>
#include <queue.h>
#include <clock.h>

/**
 * @ingroup threads
 *
 * スレッドを通常より早くスリープキューから削除する
 * @param tid  対象のスレッド
 * @return スレッドが削除されたら OK、そうでなければ SYSERR
 */
syscall unsleep(tid_typ tid)
{
    register struct thrent *thrptr;
    irqmask im;
    tid_typ next = 0;

    im = disable();

    if (isbadtid(tid))
    {
        restore(im);
        return SYSERR;
    }

    thrptr = &thrtab[tid];
    if ((thrptr->state != THRSLEEP) && (thrptr->state != THRTMOUT))
    {
        restore(im);
        return SYSERR;
    }

    next = quetab[tid].next;
    if (next < NTHREAD)
    {
        // 次のsleepスレッドの待ち時間を調整
        quetab[next].key += quetab[tid].key;
    }

    // スレッドをreadyキューから削除
    getitem(tid);
    restore(im);
    return OK;
}
