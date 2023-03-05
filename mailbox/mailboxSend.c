/**
 * @file mailboxSend.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <stddef.h>
#include <interrupt.h>
#include <mailbox.h>

/**
 * @ingroup mailbox
 *
 * 指定されたメールボックスにメッセージを送信する
 *
 * @param box
 *      メッセージを送信するメールボックスのインデックス
 *
 * @param mailmsg
 *      送信するメッセージ
 *
 * @return メッセージのエンキューに成功した場合は ::OK 、
 *         @p box に正しいメールボックスが指定されていなかった、
 *         あるいは、キューに空きが出るまで待機中にメールボックスが
 *         解放された場合は ::SYSERR
 */
syscall mailboxSend(mailbox box, int mailmsg)
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
        /* メールボックキューに空きができるまで待機 */
        wait(mbxptr->sender);

        /* メールボックスが解放されていない場合に限り処理を継続する  */
        if (MAILBOX_ALLOC == mbxptr->state)
        {
            /* このメールボックスのmailmsgキューにメッセージを書き込む */
            mbxptr->msgs[((mbxptr->start + mbxptr->count) % mbxptr->max)] =
                mailmsg;
            mbxptr->count++;

            /* mailmsgキューに新メッセージがあることを通知する */
            signal(mbxptr->receiver);

            retval = OK;
        }
    }

    restore(im);
    return retval;
}
