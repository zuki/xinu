/**
 * @file     clkhandler.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <stddef.h>
#include <queue.h>
#include <clock.h>
#include <thread.h>
#include <platform.h>

#if RTCLOCK

void wakeup(void);
int resched(void);

/**
 * @ingroup timer
 *
 * タイマー割り込み用の割り込みハンドラ関数。
 * 将来のある時点で発生する新しいタイマー割り込みをスケジュールし、
 * ::clktime と ::clkticks を更新し、スリープ中のスレッドがあれば
 * 起床させ、 そうでなければプロセッサを再スケジュールする。
 */
interrupt clkhandler(void)
{
    clkupdate(platform.clkfreq / CLKTICKS_PER_SEC);

    /* Another clock tick passes. */
    clkticks++;

    /* グローバル秒カウンタを更新する */
    if (CLKTICKS_PER_SEC == clkticks)
    {
        clktime++;
        clkticks = 0;
    }

    /* sleepqが空でない場合は、第一キーを減ずる     */
    /* キーがゼロに達したら、wakeupを呼び出す         */
    if (nonempty(sleepq) && (--firstkey(sleepq) <= 0))
    {
        wakeup();
    }
    else
    {
        resched();
    }
}

#endif /* RTCLOCK */
