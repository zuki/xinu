/**
 * @file getprio.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>

/**
 * @ingroup threads
 *
 * スレッドのスケジューリング優先度を返す
 * @param tid スレッドID
 * @return 成功の場合はスレッドの優先度、失敗の場合は SYSERR
 */
syscall getprio(tid_typ tid)
{
    int prio;
    irqmask im;

    im = disable();
    if (isbadtid(tid))
    {
        restore(im);
        return SYSERR;
    }

    prio = thrtab[tid].prio;
    restore(im);
    return prio;
}
