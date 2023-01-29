/**
 * @file signal.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>

/**
 * @ingroup semaphores
 *
 * 待機中のスレッドを1つ開放するようセマフォに伝える
 *
 * signal()は現在実行中のスレッドを再スケジュールする可能性がある。
 * そのため、 signal()は割り込みハンドラの冒頭で::resdeferに正値を
 * 設定しない限り、非リエントラントな割り込み処理から呼び出しては
 * ならない。
 *
 * @param sem
 *      シグナルを送るセマフォ
 *
 * @return
 *      成功の場合は ::OK、失敗の場合は ::SYSERR。
 *      この関数は @p sem に正しいセマフォが指定されなかった場合に
 *      だけ失敗する。
 */
syscall signal(semaphore sem)
{
    register struct sement *semptr;
    irqmask im;

    im = disable();
    if (isbadsem(sem))
    {
        restore(im);
        return SYSERR;
    }
    semptr = &semtab[sem];
    if ((semptr->count++) < 0)
    {
        ready(dequeue(semptr->queue), RESCHED_YES);
    }
    restore(im);
    return OK;
}
