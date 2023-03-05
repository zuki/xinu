/**
 * @file arpDaemon.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <arp.h>
#include <mailbox.h>

/**
 * @ingroup arp
 *
 * ARPテーブルを管理するARPデーモン
 */
thread arpDaemon(void)
{
    struct packet *pkt = NULL;

    while (TRUE)
    {
        pkt = (struct packet *)mailboxReceive(arpqueue);
        ARP_TRACE("Daemon received ARP packet");
        if (SYSERR == (int)pkt)
        {
            continue;
        }

        arpSendReply(pkt);

        /* パケット用のバッファを解放する */
        if (SYSERR == netFreebuf(pkt))
        {
            ARP_TRACE("Failed to free packet buffer");
            continue;
        }
    }

    return OK;
}
