/**
 * @file telnetOpen.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <telnet.h>
#include <device.h>
#include <interrupt.h>
#include <stddef.h>
#include <stdarg.h>

#include <stdio.h>

/**
 * @ingroup telnet
 *
 * TELNETをハードウェアデバイスに関連付ける.
 * @param devptr TELNETデバイステーブルエントリ
 * @param ap ハードウェアデバイスのデバイス番号
 * @return TELNETが正しくオープンできたら ::OK; それ以外は ::SYSERR
 */
devcall telnetOpen(device *devptr, va_list ap)
{

    struct telnet *tntptr = NULL;
    int dvnum = 0;
    irqmask im;

    im = disable();

    /* 引数apは物理ハードウェアのデバイスインデックスのはず */
    dvnum = va_arg(ap, int);
    TELNET_TRACE("Open(%d) dvnum = %d", devptr->minor, dvnum);
    if (isbaddev(dvnum))
    {
        restore(im);
        return SYSERR;
    }

    /* telnetへのポインタを設定する */
    tntptr = &telnettab[devptr->minor];

    /* telnetがオープン済みでないかチェックする */
    if ((TELNET_STATE_FREE != tntptr->state) &&
        (TELNET_STATE_ALLOC != tntptr->state))
    {
        TELNET_TRACE("state = %d", tntptr->state);
        restore(im);
        return SYSERR;
    }
    tntptr->state = TELNET_STATE_OPEN;

    /* 入力バッファを初期化する */
    tntptr->istart = 0;
    tntptr->icount = 0;
    tntptr->idelim = FALSE;

    /* 出力バッファを初期化する */
    tntptr->ocount = 0;
    tntptr->ostart = 0;

    /* フラグを初期化する */
    tntptr->flags = 0;
    tntptr->ieof = FALSE;
    tntptr->phw = (device *)&devtab[dvnum];

    /* 状態とmutexセマフォを初期化する */
    tntptr->echoState = TELNET_ECHO_SENT_WILL;
    tntptr->isem = semcreate(1);
    tntptr->osem = semcreate(1);
    /* Restore interrupts after making changes to telnet device structure */
    restore(im);
    return OK;
}
