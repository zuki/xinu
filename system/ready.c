/**
 * @file ready.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>
#include <queue.h>

/**
 * @ingroup threads
 *
 * スレッドをCPUサービスの対象とする
 * @param tid 対象のスレッド
 * @param resch RESCHED_YESの場合、再スケジュールする
 * @return スレッドをreadylistに追加したら OK、それ以外は SYSERR
 */
int ready(tid_typ tid, bool resch)
{
    register struct thrent *thrptr;

    if (isbadtid(tid))
    {
        return SYSERR;
    }

    thrptr = &thrtab[tid];
    thrptr->state = THRREADY;

    insert(tid, readylist, thrptr->prio);

    if (resch == RESCHED_YES)
    {
        resched();
    }
    return OK;
}
