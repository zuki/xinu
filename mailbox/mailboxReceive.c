/**
 * @file mailboxReceive.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <interrupt.h>
#include <mailbox.h>

/**
 * @ingroup mailbox
 *
 * 指定されたメールボックスからメッセージを受信する
 *
 * @param box
 *      メッセージを受信するメールボックスのインデックス
 *
 * @return
 *      成功した場合、デキューしたメッセージを返す。
 *      失敗した場合（@p box に割り当てられたメールボックスを指定
 *      されていない、または、メッセージの大気中にメールボックスが
 *      解放された）、::SYSERR を返す。::SYSERR と成功時の返り値を
 *      曖昧なく区別することはできないことに注意。
 */
syscall mailboxReceive(mailbox box)
{
    struct mbox *mbxptr;
    irqmask im;
    int retval;

    if (!(0 <= box && box < NMAILBOX))
    {
        return SYSERR;
    }

    mbxptr = &mboxtab[box];
    im = disable();
    retval = SYSERR;
    if (MAILBOX_ALLOC == mbxptr->state)
    {
        /* mailmsgキューにメッセージが来るまで待機する */
        wait(mbxptr->receiver);

        /* メールボックスが解放されていない場合に限り処理を継続する  */
        if (MAILBOX_ALLOC == mbxptr->state)
        {
            /* mailmsgキューの最初のメッセージを受信する */
            retval = mbxptr->msgs[mbxptr->start];

            mbxptr->start = (mbxptr->start + 1) % mbxptr->max;
            mbxptr->count--;

            /* mailmsgキューに空きができたことを通知する */
            signal(mbxptr->sender);
        }
    }

    restore(im);
    return retval;
}
