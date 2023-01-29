/**
 * @file wait.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>

/**
 * @ingroup semaphores
 *
 * セマフォで待機する
 *
 * セマフォのカウント値が正の場合は、カウント値を減じて、直ちに復帰する。
 * それ以外の場合は、現在実行中のスレッドは指定されたセマフォがsignal()
 * またはsignaln()で通知される、または、semfree()で開放されるまで、
 * スリープ状態に置かれる。
 *
 * @param sem
 *      待機するセマフォ
 *
 * @return
 *      成功の場合、::OK、失敗の場合は ::SYSERR を返す。
 *      この関数は @p sem に正しいセマフォが指定されなかった
 *      場合にだけ失敗する。
 */
syscall wait(semaphore sem)
{
    register struct sement *semptr;
    register struct thrent *thrptr;
    irqmask im;

    im = disable();
    if (isbadsem(sem))
    {
        restore(im);
        return SYSERR;
    }
    thrptr = &thrtab[thrcurrent];
    semptr = &semtab[sem];
    if (--(semptr->count) < 0)
    {
        thrptr->state = THRWAIT;
        thrptr->sem = sem;
        enqueue(thrcurrent, semptr->queue);
        resched();
    }
    restore(im);
    return OK;
}
