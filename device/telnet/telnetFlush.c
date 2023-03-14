/**
 * @file telnetFlush.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <interrupt.h>
#include <telnet.h>

/**
 * @ingroup telnet
 *
 * telnetサーバの出力バッファをフラッシュする
 * @param devptr TELNETデバイステーブルエントリ
 * @return フラッシュが成功したら ::OK; 失敗したら ::SYSERR
 */
devcall telnetFlush(device *devptr)
{
    struct telnet *tntptr;
    device *phw;
    irqmask im;

    tntptr = &telnettab[devptr->minor];
    phw = tntptr->phw;
    im = disable();

    if (NULL == phw)
    {
        restore(im);
        return SYSERR;
    }

    if (TELNET_STATE_OPEN != tntptr->state)
    {
        restore(im);
        return SYSERR;
    }

    if (tntptr->ostart > 0)
    {
        if (SYSERR ==
            (*phw->write) (phw, (void *)(tntptr->out), tntptr->ostart))
        {
            restore(im);
            return SYSERR;
        }

        tntptr->ostart = 0;
    }

    restore(im);

    return OK;
}
