/**
 * @file recvclr.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>

/**
 * @ingroup threads
 *
 * メッセージをクリアする。（あれば）待機中のメッセージを返す
 * @return メッセージがあればそのメッセージ、なければ NOBSG
 */
message recvclr(void)
{
    register struct thrent *thrptr;
    irqmask im;
    message msg;
    unsigned int cpuid;

    im = disable();

    cpuid = getcpuid();

    thrtab_acquire(thrcurrent[cpuid]);

    thrptr = &thrtab[thrcurrent[cpuid]];
    if (thrptr->hasmsg)
    {
        msg = thrptr->msg;
    }                           /* メッセージを受信する  */
    else
    {
        msg = NOMSG;
    }
    thrptr->hasmsg = FALSE;     /* メッセージフラグをリセット */

    thrtab_release(thrcurrent[cpuid]);

    restore(im);
    return msg;
}
