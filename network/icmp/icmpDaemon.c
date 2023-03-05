/**
 * @file icmpDaemon.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <icmp.h>
#include <ipv4.h>
#include <mailbox.h>

/**
 * @ingroup icmp
 *
 * ICMPエコー要求 (pingD に応答するデーモン.
 * @return 復帰しない
 */
thread icmpDaemon(void)
{
#if NNETIF

    while (TRUE)
    {
        struct packet *pkt;

        /* 1. 次のICMPエコー要求を受信する */
        pkt = (struct packet *)mailboxReceive(icmpqueue);
        ICMP_TRACE("Daemon received ICMP packet");
        ICMP_TRACE("%u bytes total; %u bytes ICMP header+data",
                   pkt->len, pkt->len - (pkt->curr - pkt->data));

        /* 2. パケットバッファを再利用してICMPエコー応答を送信する */
        if (OK != icmpEchoReply(pkt))
        {
            ICMP_TRACE("Failed to send ICMP Echo Reply.");
        }

        /* 3. パケットバッファfを解放する */
        netFreebuf(pkt);
    }
#endif /* NNETIF */
    return OK;
}
