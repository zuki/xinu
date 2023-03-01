/**
 * @file wakeup.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <thread.h>
#include <queue.h>
#include <clock.h>

#if RTCLOCK

/**
 * @ingroup threads
 *
 * スリープ時間を超えたスレッドをすべて起床させ、ready状態に移す。
 */
void wakeup(void)
{
    unsigned int cpuid;

    cpuid = getcpuid();

    while (nonempty(sleepq) && (firstkey(sleepq) <= 0))
    {
        ready(dequeue(sleepq), RESCHED_NO, cpuid);
    }

    resched();
}

#endif /* RTCLOCK */
