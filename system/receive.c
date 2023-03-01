/**
 * @file receive.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>

/**
 * @ingroup threads
 *
 * 受信する - メッセージを待ってそれを返す
 * @return メッセージ
 */
message receive(void)
{
    register struct thrent *thrptr;
    irqmask im;
    message msg;
    unsigned int cpuid;

    im = disable();

    cpuid = getcpuid();

    thrptr = &thrtab[thrcurrent[cpuid]];
    if (FALSE == thrptr->hasmsg)
    {                           /* メッセージがなければ来るのを待つ */
        thrtab_acquire(thrcurrent[cpuid]);
        thrptr->state = THRRECV;
        thrtab_release(thrcurrent[cpuid]);
        resched();
    }

    thrtab_acquire(thrcurrent[cpuid]);

    msg = thrptr->msg;          /* メッセージを受信する            */
    thrptr->hasmsg = FALSE;     /* メッセフラグをリセットする      */

    thrtab_release(thrcurrent[cpuid]);

    restore(im);
    return msg;
}
