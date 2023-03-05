/**
 * @file rtDaemon.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <mailbox.h>
#include <network.h>
#include <route.h>
#include <thread.h>

/**
 * @ingroup route
 *
 * パケットをルーティングするルートデーモンスレッド.
 * @return このスレッドは復帰しない
 */
thread rtDaemon(void)
{
    struct packet *pkt = NULL;

    while (TRUE)
    {
        /* 1. rtqueueメールボックスからパケットを受信する */
        pkt = (struct packet *)mailboxReceive(rtqueue);
        RT_TRACE("Daemon received packet");
        if (SYSERR == (int)pkt)
        {
            RT_TRACE("Daemon received packet has an error");
            continue;
        }
        /* 2. パケットをルーティングする */
        rtSend(pkt);
        /* 3. パケットを破棄する */
        if (SYSERR == netFreebuf(pkt))
        {
            RT_TRACE("Failed to free packet buffer");
            continue;
        }
    }

    return OK;
}
