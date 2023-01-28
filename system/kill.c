/**
 * @file kill.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>
#include <queue.h>
#include <memory.h>
#include <safemem.h>

extern void xdone(void);

/**
 * @ingroup threads
 *
 * スレッドをKillし、システムから削除する
 * @param tid 対象のスレッド
 * @return 成功の場合は OK、そうでなければ SYSERR
 */
syscall kill(tid_typ tid)
{
    register struct thrent *thrptr;     /* thread control block */
    irqmask im;

    im = disable();
    if (isbadtid(tid) || (NULLTHREAD == tid))
    {
        restore(im);
        return SYSERR;
    }
    thrptr = &thrtab[tid];

    if (--thrcount <= 1)
    {
        xdone();
    }

#ifdef UHEAP_SIZE
    /* 使用したメモリ領域を回収する */
    memRegionReclaim(tid);
#endif                          /* UHEAP_SIZE */

    // 親に通知
    send(thrptr->parent, tid);
    // スタックを開放
    stkfree(thrptr->stkbase, thrptr->stklen);

    switch (thrptr->state)
    {
    case THRSLEEP:
        unsleep(tid);
        thrptr->state = THRFREE;
        break;
    case THRCURR:
        thrptr->state = THRFREE;        /* 自殺 */
        resched();

    case THRWAIT:
        semtab[thrptr->sem].count++;

    case THRREADY:
        getitem(tid);           /* readyキューから削除 */

    default:
        thrptr->state = THRFREE;
    }

    restore(im);
    return OK;
}
