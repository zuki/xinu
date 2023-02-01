/**
 * @file     ethloopOpen.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <bufpool.h>
#include <device.h>
#include <ethloop.h>
#include <interrupt.h>
#include <semaphore.h>
#include <stdlib.h>

/**
 * @ingroup ethloop
 *
 * ethloopデバイスをオープンする.
 * @param devptr ethloop用のデバイステーブルエントリ
 * @return ethloopのオープンに成功したら OK、そうでなければ SYSERR
 */
devcall ethloopOpen(device *devptr)
{
    struct ethloop *elpptr;
    irqmask im;
    int retval = SYSERR;

    elpptr = &elooptab[devptr->minor];
    im = disable();

    /* ethloopはオープン済みでないこと */
    if (ELOOP_STATE_FREE != elpptr->state)
    {
        goto out_restore;
    }

    /* フラグと統計をクリアする */
    elpptr->flags = 0;
    elpptr->nout = 0;

    /* セマフォを作成する */
    elpptr->sem = semcreate(0);
    if (SYSERR == (int)elpptr->sem)
    {
        goto out_restore;
    }

    elpptr->hsem = semcreate(0);
    if (SYSERR == (int)elpptr->hsem)
    {
        goto out_free_sem;
    }

    /* バッファを初期化する */
    bzero(elpptr->buffer, sizeof(elpptr->buffer));
    bzero(elpptr->pktlen, sizeof(elpptr->pktlen));
    elpptr->index = 0;
    elpptr->hold = NULL;
    elpptr->holdlen = 0;
    elpptr->count = 0;

    /* バッファプールを割り当てる  */
    elpptr->poolid = bfpalloc(ELOOP_BUFSIZE, ELOOP_NBUF);
    if (SYSERR == elpptr->poolid)
    {
        goto out_free_hsem;
    }

    /* ethloopレコードをデバイステーブルエントリに結びつけ、
     * オープン済みとマークする */
    elpptr->state = ELOOP_STATE_ALLOC;
    elpptr->dev = devptr;

    /* 成功 */
    retval = OK;
    goto out_restore;

out_free_hsem:
    semfree(elpptr->hsem);
out_free_sem:
    semfree(elpptr->sem);
out_restore:
    restore(im);
    return retval;
}
