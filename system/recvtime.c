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

    if (maxwait < 0)
    {
        return SYSERR;
    }
    im = disable();
    thrptr = &thrtab[thrcurrent];
    if (FALSE == thrptr->hasmsg)
    {
#if RTCLOCK
        // sleepqに入れる
        if (SYSERR == insertd(thrcurrent, sleepq, maxwait))
        {
            restore(im);
            return SYSERR;
        }
        thrtab[thrcurrent].state = THRTMOUT;
        resched();
#else
        restore(im);
        return SYSERR;
#endif
    }

    if (thrptr->hasmsg)
    {
        msg = thrptr->msg;      /* メッセージを取得              */
        thrptr->hasmsg = FALSE; /* メッセージフラグをリセット    */
    }
    else
    {
        msg = TIMEOUT;
    }
    restore(im);
    return msg;
}
