/**
 * @file etherRead.c
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <bufpool.h>
#include <ether.h>
#include <interrupt.h>
#include <string.h>

/**
 * @ingroup ether_lan7800
 *  LAN7800用の etherRead() の実装. この関数のドキュメントはether.hを参照 */
devcall etherRead(device *devptr, void *buf, uint len)
{
    irqmask im;
    struct ether *ethptr;
    struct ethPktBuffer *pkt;

    im = disable();

    /* デバイスはupしていること */
    ethptr = &ethertab[devptr->minor];
    if (ethptr->state != ETH_STATE_UP)
    {
        restore(im);
        return SYSERR;
    }

    /* ethptr->in 循環キューに受信パケットが現れるのを待つ */
    wait(ethptr->isema);

    /* 受信パケットを循環キューから削除する  */
    pkt = ethptr->in[ethptr->istart];
    ethptr->istart = (ethptr->istart + 1) % ETH_IBLEN;
    ethptr->icount--;

    /* TODO: ここで（memcpy() の前に）割り込みを復元したいがまだできない。
     * lan7800_rx_complete() は icount < ETH_IBLEN であればバッファが
     * 利用可能であると期待するからである。したがって icount を減分したので、
     * 実際に対応するバッファを解放するまで割り込みを復元することはできない
     */

    /* データをパケットバッファからコピーする。要求されたバイト数だけ
     * コピーするよう注意する */
    if (pkt->length < len)
    {
        len = pkt->length;
    }
    memcpy(buf, pkt->buf, len);

    /* パケットバッファをプールに戻し、受信したパケットの長さを返す */
    buffree(pkt);
    restore(im);
    return len;
}
