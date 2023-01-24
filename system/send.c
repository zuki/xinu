/**
 * @file send.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>

/**
 * @ingroup threads
 *
 * メッセージを別のスレッドに送信する
 * @param tid 受け手のスレッドID
 * @param msg メッセージの内容
 * @return 成功の場合は OK、失敗の場合は SYSERR
 */
syscall send(tid_typ tid, message msg)
{
    register struct thrent *thrptr;
    irqmask im;

    im = disable();
    if (isbadtid(tid))
    {
        restore(im);
        return SYSERR;
    }
    thrptr = &thrtab[tid];
    if ((THRFREE == thrptr->state) || thrptr->hasmsg)   // すでのメッセージがあればエラー
    {
        restore(im);
        return SYSERR;
    }
    thrptr->msg = msg;          /* メッセージを配信する           */
    thrptr->hasmsg = TRUE;      /* メッセージフラグを立てる       */

    /* 受信者が待機中の場合は起動する */
    if (THRRECV == thrptr->state)
    {
        ready(tid, RESCHED_YES);
    }
    else if (THRTMOUT == thrptr->state)
    {
        unsleep(tid);
        ready(tid, RESCHED_YES);
    }
    restore(im);
    return OK;
}
