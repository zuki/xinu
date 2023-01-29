/**
 * @file signaln.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>

/**
 * @ingroup semaphores
 *
 * 待機中のスレッドを @p count 個、開放するようセマフォに伝える
 *
 * signal()は現在実行中のスレッドを再スケジュールする可能性がある。
 * そのため、 signal()は割り込みハンドラの冒頭で::resdeferに正値を
 * 設定しない限り、非リエントラントな割り込み処理から呼び出しては
 * ならない。
 *
 * @param sem
 *      シグナルを送るセマフォ
 * @param count
 *      シグナルを送る回数。正値でなければならない。
 *
 * @return
 *      ::OK on success, ::SYSERR on failure.  This function can only fail if @p
 *      成功の場合は ::OK、失敗の場合は ::SYSERR。
 *      この関数は @p sem に正しいセマフォが指定されなかった、または、
 *      @p count が正値でなかった場合に失敗する。
 */
syscall signaln(semaphore sem, int count)
{
    register struct sement *semptr;
    irqmask im;

    im = disable();
    if (isbadsem(sem) || (count <= 0))
    {
        restore(im);
        return SYSERR;
    }
    semptr = &semtab[sem];
    for (; count > 0; count--)
    {
        if ((semptr->count++) < 0)
        {
            ready(dequeue(semptr->queue), RESCHED_NO);
        }
    }
    resched();
    restore(im);
    return OK;
}
