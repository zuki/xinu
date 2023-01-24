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

    im = disable();
    thrptr = &thrtab[thrcurrent];
    if (FALSE == thrptr->hasmsg)
    {                           /* メッセージがなければ来るのを待つ */
        thrptr->state = THRRECV;
        resched();
    }
    msg = thrptr->msg;          /* メッセージを受信する            */
    thrptr->hasmsg = FALSE;     /* メッセフラグをリセットする      */
    restore(im);
    return msg;
}
