/**
 * @file ready.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>
#include <queue.h>
#include <core.h>

/**
 * @ingroup threads
 *
 * スレッドをCPUサービスの対象とする
 * @param tid 対象のスレッド
 * @param resch RESCHED_YESの場合、再スケジュールする
 * @return スレッドをreadylistに追加したら OK、それ以外は SYSERR
 */
int ready(tid_typ tid, bool resch, unsigned int core)
{
    register struct thrent *thrptr;

    if (isbadtid(tid))
    {
        return SYSERR;
    }

    thrtab_acquire(tid);

    thrptr = &thrtab[tid];
    thrptr->state = THRREADY;

    /* コアアフィニティがセットされていない場合は、現在このコードを
       実行しているコアにアフィニティをセットする（ほとんど場合は0） */
    unsigned int cpuid;
    cpuid = getcpuid();
    if (-1 == thrptr->core_affinity)
    {
        thrptr->core_affinity = core;
    }

    thrtab_release(tid);

    if (SYSERR == insert(tid, readylist[thrptr->core_affinity], thrptr->prio))
    {
        return SYSERR;
    }

    if ((resch == RESCHED_YES) && (thrptr->core_affinity == cpuid))
    {
        resched();
    }

    return OK;
}
