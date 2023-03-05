/**
 * @file rtRecv.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <interrupt.h>
#include <mailbox.h>
#include <network.h>
#include <route.h>

/**
 * @ingroup route
 * ルートパケットを受信する（ルートキューに入れる）.
 * @param pkt ルートパケット
 * @return 成功したら OK; それ以外は SYSRR
 */
syscall rtRecv(struct packet *pkt)
{
    irqmask im;

    /* 1. 引数のエラーチェック */
    if (NULL == pkt)
    {
        return SYSERR;
    }

    /* 2. ルートテーブルが満杯の場合はパケットを破棄する */
    im = disable();
    if (mailboxCount(rtqueue) >= RT_NQUEUE)
    {
        restore(im);
        RT_TRACE("Route queue full");
        netFreebuf(pkt);
        return OK;
    }

    /* 3. パケットをキューに置く */
    if (SYSERR == mailboxSend(rtqueue, (int)pkt))
    {
        restore(im);
        RT_TRACE("Failed to enqueue packet");
        netFreebuf(pkt);
        return SYSERR;
    }

    restore(im);
    RT_TRACE("Enqueued packet for routing");
    return OK;
}
