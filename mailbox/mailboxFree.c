/**
 * @file mailboxFree.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <mailbox.h>
#include <memory.h>

/**
 * @ingroup mailbox
 *
 * 指定されたメールボックスを解放する
 *
 * @param box
 *      解放するメールボックスのインデックス
 *
 * @return
 *      メールボックスの解放に成功した場合 ::OK 、
 *      @p box に正しいメールボックスのインデックスが指定
 *      されなかった場合 ::SYSERR
 */
syscall mailboxFree(mailbox box)
{
    struct mbox *mbxptr;
    int retval;

    if (!(0 <= box && box < NMAILBOX))
    {
        return SYSERR;
    }

    mbxptr = &mboxtab[box];

    /* 他のスレッドがメールボックステーブルの編集を終えるまで待機 */
    wait(mboxtabsem);

    if (MAILBOX_ALLOC == mbxptr->state)
    {
        /* メールボックスがもはや割り当てられていないとマークする  */
        mbxptr->state = MAILBOX_FREE;

        /* このメールボックスに関係するセマフォを解放する */
        semfree(mbxptr->sender);
        semfree(mbxptr->receiver);

        /* メッセージキューに使用したメモリを解放する */
        memfree(mbxptr->msgs, sizeof(int) * (mbxptr->max));

        retval = OK;
    }
    else
    {
        /* メールボックスは割り当てられていない  */
        retval = SYSERR;
    }

    /* このスレッドがメールボックステーブルの編集を終えたことを通知する */
    signal(mboxtabsem);

    return retval;
}
