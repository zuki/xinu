/**
 * @file resume.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>
#include <queue.h>

/**
 * @ingroup threads
 *
 * スレッドの一時停止を止め、readylistに置いて、リスケジュールする
 * @param tid 対象のスレッド
 * @return 優先度、または SYSERR
 */
syscall resume(tid_typ tid)
{
    register struct thrent *thrptr;     /* thread control block  */
    irqmask im;
    int prio;
    unsigned int cpuid;

    im = disable();
    thrptr = &thrtab[tid];
    if (isbadtid(tid) || (thrptr->state != THRSUSP))
    {
        restore(im);
        return SYSERR;
    }

    cpuid = getcpuid();
    prio = thrptr->prio;
    ready(tid, RESCHED_YES, cpuid);
    restore(im);
    return prio;
}
