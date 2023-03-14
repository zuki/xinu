/**
 * @file mailboxInit.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <mailbox.h>

struct mbox mboxtab[NMAILBOX];
semaphore mboxtabsem;

/**
 * @ingroup mailbox
 *
 * メールボックス構造体を初期化する
 *
 * @return
 *      すべてのメールボックスの初期化に成功した場合は ::OK;
 *      それ以外は ::SYSERR.
 */
syscall mailboxInit(void)
{
    uint i;

    /* すべてのメールボックスの状態に MAILBOX_FREE をセットする */
    for (i = 0; i < NMAILBOX; i++)
    {
        mboxtab[i].state = MAILBOX_FREE;
    }

    mboxtabsem = semcreate(1);
    if (SYSERR == mboxtabsem)
    {
        return SYSERR;
    }

    return OK;
}
