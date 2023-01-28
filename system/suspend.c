/**
 * @file suspend.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>
#include <queue.h>

/**
 * @ingroup threads
 *
 * スレッドを一時停止して、ハイバネーションに置く
 * @param tid 対象のスレッド
 * @return 優先度、または SYSERR
 */
syscall suspend(tid_typ tid)
{
    register struct thrent *thrptr;     /* thread control block  */
    irqmask im;
    int prio;

    im = disable();
    if (isbadtid(tid) || (NULLTHREAD == tid))
    {
        restore(im);
        return SYSERR;
    }
    thrptr = &thrtab[tid];
    if ((thrptr->state != THRCURR) && (thrptr->state != THRREADY))
    {
        restore(im);
        return SYSERR;
    }
    // raady状態にある場合は、readylistから削除
    if (THRREADY == thrptr->state)
    {
        getitem(tid);           /* キューから削除 */
        thrptr->state = THRSUSP;
    }
    // 実行中の場合は、suspend状態にしてリスケジュール
    else
    {
        thrptr->state = THRSUSP;
        resched();
    }
    prio = thrptr->prio;
    restore(im);
    return prio;
}
