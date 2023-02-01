/**
 * @file     ethloopWrite.c
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
 * Ethernetループバックデバイスにデータを書き出す.
 * 成功した場合、続いて呼び出される ethloopRead() により
 * 読み込みが可能にになる。
 *
 * @param devptr
 *      ethloop用のデバイステーブルエントリへのポインタ
 *
 * @param buf
 *      書き出すデータが格納されたバッファ
 *
 * @param len
 *      書き出すデータの（バイト単位の）長さ
 *
 * @return
 *      成功の場合、書き出したバイト数を返す。これは @p len に等しい。
 *      それ以外は SYSERR を返す。
 */
devcall ethloopWrite(device *devptr, const void *buf, uint len)
{
    struct ethloop *elpptr;
    irqmask im;
    int index;
    char *pkt;

    elpptr = &elooptab[devptr->minor];

    /* パケットが小さすぎず、大きすぎないこと */
    if ((len < ELOOP_LINKHDRSIZE) || (len > ELOOP_BUFSIZE))
    {
        return SYSERR;
    }

    im = disable();

    /* ethloopがオープンされていること  */
    if (ELOOP_STATE_ALLOC != elpptr->state)
    {
        restore(im);
        return SYSERR;
    }

    /* 履きフラグがセットされている場合はパケットを破棄する */
    if (elpptr->flags & (ELOOP_FLAG_DROPNXT | ELOOP_FLAG_DROPALL))
    {
        elpptr->flags &= ~ELOOP_FLAG_DROPNXT;
        restore(im);
        return len;
    }

    /* バッファスペースを割り当てる。ブロックされるのでプールIDが
     * 衝突する場合のみ失敗する  */
    pkt = (char *)bufget(elpptr->poolid);
    if (SYSERR == (int)pkt)
    {
        restore(im);
        return SYSERR;
    }

    /* 指定されたバッファを割り当てたバッファにコピーする */
    memcpy(pkt, buf, len);

    /* 該当するフラグがセットされている場合は次のパケットを保持する */
    if (elpptr->flags & ELOOP_FLAG_HOLDNXT)
    {
        elpptr->flags &= ~ELOOP_FLAG_HOLDNXT;
        if (elpptr->hold != NULL)
        {
            buffree(elpptr->hold);
        }
        elpptr->hold = pkt;
        elpptr->holdlen = len;
        restore(im);
        signal(elpptr->hsem);
        return len;
    }

    /* 順分なバッファスペースがあること（常にそうであるはず）  */
    if (elpptr->count >= ELOOP_NBUF)
    {
        buffree(pkt);
        restore(im);
        return SYSERR;
    }

    index = (elpptr->count + elpptr->index) % ELOOP_NBUF;

    /* バッファに追加する */
    elpptr->buffer[index] = pkt;
    elpptr->pktlen[index] = len;
    elpptr->count++;

    /* 書き込んだパケット数を増分する */
    elpptr->nout++;

    restore(im);

    signal(elpptr->sem);

    return len;
}
