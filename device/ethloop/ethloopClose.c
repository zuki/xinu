/**
 * @file ethloopClose.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <bufpool.h>
#include <device.h>
#include <ethloop.h>
#include <interrupt.h>
#include <semaphore.h>

/**
 * @ingroup ethloop
 *
 * ethloopデバイスをクローズする.
 * @param devptr ethloop用のデバイステーブルエントリ
 * @return ethloopのクローズに成功したら OK、そうでなければ SYSERR
 */
devcall ethloopClose(device *devptr)
{
    struct ethloop *elpptr;
    irqmask im;

    elpptr = &elooptab[devptr->minor];
    im = disable();

    /* ethloopはオープン済みであること */
    if (ELOOP_STATE_ALLOC != elpptr->state)
    {
        restore(im);
        return SYSERR;
    }

    /* セマフォを解放する */
    semfree(elpptr->sem);
    semfree(elpptr->hsem);

    /* バッファプールを解放する */
    bfpfree(elpptr->poolid);

    /* オープンされていないとマークする */
    elpptr->dev = NULL;
    elpptr->state = ELOOP_STATE_FREE;
    restore(im);
    return OK;
}
