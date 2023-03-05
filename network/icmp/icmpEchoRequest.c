/**
 * @file icmpEchoRequest.c
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <icmp.h>
#include <stdlib.h>
#include <clock.h>
#include <interrupt.h>

/**
 * @ingroup icmp
 *
 * ICMPエコー要求 (ping) を送信する.
 * @param dst あて先アドレス
 * @param id  pingストリーム識別子
 * @param seq シーケンス番号
 * @return パケットが送信されたら OK; それ以外は SYSERR
 */
syscall icmpEchoRequest(struct netaddr *dst, ushort id, ushort seq)
{
    struct packet *pkt;
    struct icmpEcho *echo;
    int result;
    struct netaddr src;
    irqmask im;

    ICMP_TRACE("echo request(%d, %d)", id, seq);
    pkt = netGetbuf();
    if (SYSERR == (int)pkt)
    {
        ICMP_TRACE("Failed to acquire packet buffer");
        return SYSERR;
    }

    pkt->len = sizeof(struct icmpEcho);
    pkt->curr -= pkt->len;

    echo = (struct icmpEcho *)pkt->curr;
    echo->id = hs2net(id);
    echo->seq = hs2net(seq);
    /* オプションのデータペイロードには発信と到着のタイムスタンプを
     * 秒、ミリ秒、クロックサイクルで入力するスペースがある */
    im = disable();
    echo->timecyc = hl2net(clkcount());
    echo->timetic = hl2net(clkticks);
    echo->timesec = hl2net(clktime);
    restore(im);
    echo->arrivcyc = 0;
    echo->arrivtic = 0;
    echo->arrivsec = 0;

    ICMP_TRACE("Sending Echo Request id = %d, seq = %d, time = %lu.%lu",
               net2hs(echo->id), net2hs(echo->seq),
               net2hl(echo->timesec), net2hl(echo->timetic));

    src.type = 0;

    result = icmpSend(pkt, ICMP_ECHO, 0, sizeof(struct icmpEcho), &src, dst);
    netFreebuf(pkt);
    return result;
}
