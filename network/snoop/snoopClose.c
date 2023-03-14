/** @file snoopClose.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <interrupt.h>
#include <network.h>
#include <snoop.h>

/**
 * @ingroup snoop
 *
 * ネットワークデバイスまたはファイルからのキャプチャをクローズする.
 * @param cap キャプチャ構造体へのポインタ
 * @return クローズに成功したら ::OK; それ以外は ::SYSERR
 */
int snoopClose(struct snoop *cap)
{
    struct packet *pkt;
    int i;
    irqmask im;

    /* 引数のエラーチェック */
    if (NULL == cap)
    {
        return SYSERR;
    }

    /* すべてのネットワークインタフェースからキャプチャを削除する */
    im = disable();
#if NNETIF
    for (i = 0; i < NNETIF; i++)
    {
        if (netiftab[i].capture == cap)
        {
            netiftab[i].capture = NULL;
        }
    }
#endif
    restore(im);

    /* キューにあるパケットを解放する */
    while (mailboxCount(cap->queue) > 0)
    {
        pkt = (struct packet *)mailboxReceive(cap->queue);
        if (SYSERR == netFreebuf(pkt))
        {
            return SYSERR;
        }
    }

    /* メールボックスを解放する */
    if (SYSERR == mailboxFree(cap->queue))
    {
        return SYSERR;
    }

    return OK;
}
