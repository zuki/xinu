/**
 * @file kill.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>
#include <queue.h>
#include <memory.h>
#include <safemem.h>
#include <core.h>

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
    unsigned int cpuid;

    cpuid = getcpuid();

    im = disable();
    if (isbadtid(tid) || (NULLTHREAD == tid) || (NULLTHREAD1 == tid) ||
            (NULLTHREAD2 == tid) || (NULLTHREAD3 == tid))
    {
        restore(im);
        return SYSERR;
    }

    thrtab_acquire(tid);

    thrptr = &thrtab[tid];

    /* 異なるコアで実行しているプロセスはkillできない */
    if (thrptr->core_affinity != cpuid)
    {
        thrtab_release(tid);
        restore(im);
        return SYSERR;
    }

    thrtab_release(tid);

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

    thrtab_acquire(tid);
    thrtab->core_affinity = cpuid;
    thrtab_release(tid);

    switch (thrptr->state)
    {
    case THRSLEEP:
        unsleep(tid);
        thrtab_acquire(tid);
        thrptr->state = THRFREE;
        thrtab_release(tid);
        break;
    case THRCURR:
        thrtab_acquire(tid);
        thrptr->state = THRFREE;        /* 自殺 */
        thrtab_release(tid);
        resched();

    case THRWAIT:
        semtab_acquire(thrptr->sem);
        semtab[thrptr->sem].count++;
        semtab_release(thrptr->sem);

    case THRREADY:
        getitem(tid);           /* readyキューから削除 */

    default:
        thrtab_acquire(tid);
        thrptr->state = THRFREE;
        thrtab_release(tid);
    }

    restore(im);
    return OK;
}
