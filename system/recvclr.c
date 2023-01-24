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

    im = disable();
    thrptr = &thrtab[thrcurrent];
    if (thrptr->hasmsg)
    {
        msg = thrptr->msg;
    }                           /* メッセージを受信する  */
    else
    {
        msg = NOMSG;
    }
    thrptr->hasmsg = FALSE;     /* メッセージフラグをリセット */
    restore(im);
    return msg;
}
