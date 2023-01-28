/**
 * @file chprio.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>

/**
 * @ingroup threads
 *
 * スレッドのスケジューリング優先度を変更する
 * @param tid 対象のスレッド
 * @param newprio 新しい優先度
 * @return スレッドの変更前の優先度
 */
syscall chprio(tid_typ tid, int newprio)
{
    register struct thrent *thrptr;     /* thread control block */
    irqmask im;
    int oldprio;

    im = disable();
    if (isbadtid(tid))
    {
        restore(im);
        return SYSERR;
    }
    thrptr = &thrtab[tid];
    oldprio = thrptr->prio;
    thrptr->prio = newprio;
    restore(im);
    return oldprio;
}
