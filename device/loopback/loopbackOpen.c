/**
 * @file     loopbackOpen.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <loopback.h>
#include <stdlib.h>
#include <interrupt.h>

/**
 * @ingroup loopback
 *
 * ループバックデバイスをオープンする.
 * @param devptr ループバック用デバイステーブルエントリ
 * @return ループバックのオープンに成功したら OK、それ以外は SYSERR
 */
devcall loopbackOpen(device *devptr)
{
    struct loopback *lbkptr;
    irqmask im;

    lbkptr = &looptab[devptr->minor];

    im = disable();
    /* ループバックがすでにお～っ分済みかチェックする */
    if (LOOP_STATE_FREE != lbkptr->state)
    {
        restore(im);
        return SYSERR;
    }

    /* 新たにセマフォを作成する */
    lbkptr->sem = semcreate(0);

    if (SYSERR == lbkptr->sem)
    {
        restore(im);
        return SYSERR;
    }

    /* バッファを0クリア */
    bzero(lbkptr->buffer, LOOP_BUFFER);

    /* フラグを0で初期化 */
    lbkptr->flags = 0;

    lbkptr->state = LOOP_STATE_ALLOC;

    restore(im);
    return OK;
}
