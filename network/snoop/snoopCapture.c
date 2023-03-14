/** @file snoopCapture.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <snoop.h>

/**
 * @ingroup snoop
 *
 * ネットワークインタフェースからのネットワークパケットをキャプチャする.
 * @return キャプチャが成功したら ::OK; それ以外は ::SYSERR
 */
int snoopCapture(struct snoop *cap, struct packet *pkt)
{
    struct packet *buf;
    int len;

    /* 引数のエラーチェック */
    if ((NULL == cap) || (NULL == pkt))
    {
        return SYSERR;
    }

    SNOOP_TRACE("Capturing packet");

    /* パケットのキャプチャ回数を増分する */
    cap->ncap++;

    /* パケットがキャプチャフィルタにマッチするかチェックし、
     * 一致しなければOKを返す */
    if (FALSE == snoopFilter(cap, pkt))
    {
        SNOOP_TRACE("Packet does not match filter");
        return OK;
    }

    /* フィルタにマッチしたパケット数を増分する */
    cap->nmatch++;

    /* パケットを収めるバッファを取得する */
    buf = netGetbuf();
    if (SYSERR == (int)buf)
    {
        SNOOP_TRACE("Failed to get buffer");
        return SYSERR;
    }

    /* パケットヘッダーをバッファにコピーする */
    memcpy(buf, pkt, sizeof(struct packet));

    /* パケットの内容をバッファにコピーする */
    len = pkt->len;
    if (len > cap->caplen)
    {
        len = cap->caplen;
    }
    memcpy(buf->data, pkt->curr, len);
    buf->curr = buf->data;

    /* パケットをキューに入れる */
    if (mailboxCount(cap->queue) >= SNOOP_QLEN)
    {
        netFreebuf(buf);
        cap->novrn++;
        SNOOP_TRACE("Capture queue full");
        return SYSERR;
    }
    if (SYSERR == mailboxSend(cap->queue, (int)buf))
    {
        netFreebuf(buf);
        SNOOP_TRACE("Failed to enqueue packet");
        return SYSERR;
    }

    return OK;
}
