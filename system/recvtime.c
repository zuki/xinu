/**
 * @file recvtime.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <conf.h>
#include <stddef.h>
#include <thread.h>
#include <clock.h>

/**
 * @ingroup threads
 *
 * メッセージの受信またはタイムアウトを待って、結果を返す
 * @param  maxwait タイムアウトになるまで待機するtick数
 * @return 受信できたら msg、メッセージがなかったら TIMEOUT
 */
message recvtime(int maxwait)
{
    register struct thrent *thrptr;
    irqmask im;
    message msg;
    unsigned int cpuid;

    cpuid = getcpuid();

    if (maxwait < 0)
    {
        return SYSERR;
    }
    im = disable();
    thrptr = &thrtab[thrcurrent[cpuid]];
    if (FALSE == thrptr->hasmsg)
    {
#if RTCLOCK
        // sleepqに入れる
        if (SYSERR == insertd(thrcurrent[cpuid], sleepq, maxwait))
        {
            restore(im);
            return SYSERR;
        }

        thrtab_acquire(thrcurrent[cpuid]);
        thrtab[thrcurrent[cpuid]].state = THRTMOUT;
        thrtab_release(thrcurrent[cpuid]);
        resched();
#else
        restore(im);
        return SYSERR;
#endif
    }

    thrtab_acquire(thrcurrent[cpuid]);

    if (thrptr->hasmsg)
    {
        msg = thrptr->msg;      /* メッセージを取得              */
        thrptr->hasmsg = FALSE; /* メッセージフラグをリセット    */
    }
    else
    {
        msg = TIMEOUT;
    }

    thrtab_release(thrcurrent[cpuid]);

    restore(im);
    return msg;
}
