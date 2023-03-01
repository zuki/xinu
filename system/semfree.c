/**
 * @file semfree.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>

/**
 * @ingroup semaphores
 *
 * セマフォを開放する。スレッドがこのセマフォを待っている場合でも実行できる。
 * その場合、スレッドは開放され実行可能になる。ただし、そのようなスレッドは
 * もはや存在しないセマフォでwait()している状態から復帰するので、もうそれを
 * 確保できないかもしれないという仮定があるので、注意が必要である。
 *
 * @param sem
 *      開放するセマフォ（semcreate()で割り当てられた）
 *
 * @return
 *      @p sem が有効なセマフォを指定していない場合は、::SYSERR, そうでなければ ::OK
 */
syscall semfree(semaphore sem)
{
    register struct sement *semptr;
    irqmask im;
    tid_typ tid;
    unsigned int cpuid;

    cpuid = getcpuid();

    im = disable();
    if (isbadsem(sem))
    {
        restore(im);
        return SYSERR;
    }

    semtab_acquire(sem);
    semptr = &semtab[sem];
    while (nonempty(semptr->queue))
    {
        tid = dequeue(semptr->queue);   /* 待機中のスレッドを開放する */
        ready(tid, RESCHED_NO, cpuid);
    }

    semptr->count = 0;
    semptr->state = SFREE;
    semtab_release(sem);

    restore(im);
    return OK;
}
