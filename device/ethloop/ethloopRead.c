/**
 * @file     ethloopRead.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <bufpool.h>
#include <device.h>
#include <ethloop.h>
#include <interrupt.h>
#include <stddef.h>
#include <string.h>

/**
 * @ingroup ethloop
 *
 * Ethernetループバックデバイスからデータを読み込む.
 * ethloopWrite() を呼び出すことによりデータが利用可能に
 * なるまでブロックされる。
 *
 * @param devptr
 *      ethloop用のデバイステーブルエントリへのポインタ
 *
 * @param buf
 *      読み込んだデータを置くバッファ
 *
 * @param len
 *      読み込むデータの（バイト単位の）最大長
 *
 * @return
 *      成功の場合、読み込んだバイト数を返す。これは @p len 以下である。
 *      それ以外は SYSERR を返す。
 */
devcall ethloopRead(device *devptr, void *buf, uint len)
{
    struct ethloop *elpptr;
    irqmask im;
    char *pkt;
    int pktlen;

    elpptr = &elooptab[devptr->minor];

    im = disable();
    if (ELOOP_STATE_ALLOC != elpptr->state)
    {
        restore(im);
        return SYSERR;
    }

    /* バッファにパケットが置かれるまで待機する */
    wait(elpptr->sem);

    pkt = elpptr->buffer[elpptr->index];
    pktlen = elpptr->pktlen[elpptr->index];
    elpptr->buffer[elpptr->index] = NULL;
    elpptr->pktlen[elpptr->index] = 0;
    elpptr->count--;
    elpptr->index = (elpptr->index + 1) % ELOOP_NBUF;
    restore(im);

    if (len < pktlen)
    {
        pktlen = len;
    }

    memcpy(buf, pkt, pktlen);
    buffree(pkt);

    return pktlen;
}
