/**
 * @file sleep.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <kernel.h>
#include <stddef.h>
#include <interrupt.h>
#include <thread.h>
#include <queue.h>
#include <clock.h>

/**
 * @ingroup threads
 *
 * 指定されたミリ秒の間プロセッサを明け渡して、他のスレッドを
 * スケジューリングできるようにする
 *
 * @param ms スリープするミリ秒数
 *
 * @return
 *      成功した場合、スレッドは指定したミリ秒数だけスリープして
 *      ::OK を返す。そうでない場合は、::SYSERR を返す。
 *      システムタイマーがサポートされていない場合は、常に
 *      ::SYSERR を返す
 */
syscall sleep(uint ms)
{
#if RTCLOCK
    irqmask im;
    int ticks = 0;
    unsigned int cpuid;

    cpuid = getcpuid();

    ticks = (ms * CLKTICKS_PER_SEC) / 1000;

    im = disable();
    if (ticks > 0)
    {
        if (SYSERR == insertd(thrcurrent[cpuid], sleepq, ticks))
        {
            restore(im);
            return SYSERR;
        }
        thrtab_acquire(thrcurrent[cpuid]);
        thrtab[thrcurrent[cpuid]].state = THRSLEEP;
        thrtab_release(thrcurrent[cpuid]);
    }

    resched();
    restore(im);
    return OK;
#else
    return SYSERR;
#endif
}
