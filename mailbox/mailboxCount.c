/**
 * @file mailboxCount.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <interrupt.h>
#include <mailbox.h>

/**
 * @ingroup mailbox
 *
 * 指定されたメールボックスの未使用のメッセージの数を調べる
 *
 * @param box
 *      未使用のメッセージ数を調べるメールボックスのインデックス
 *
 * @return
 *      メールボックスのメッセージ数、@p box に割り当て済みの
 *      正しいメールボックスが指定されなかった場合は ::SYSERR
 */
syscall mailboxCount(mailbox box)
{
    const struct mbox *mbxptr;
    irqmask im;
    int retval;

    if (!(0 <= box && box < NMAILBOX))
    {
        return SYSERR;
    }

    mbxptr = &mboxtab[box];
    im = disable();
    if (MAILBOX_ALLOC == mbxptr->state)
    {
        retval = mbxptr->count;
    }
    else
    {
        retval = SYSERR;
    }
    restore(im);
    return retval;
}
