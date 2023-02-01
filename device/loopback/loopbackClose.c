/**
 * @file loopbackClose.c
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
 * ループバックデバイスをクローズする.
 * @param devptr ループバック用デバイステーブルエントリ
 * @return ループバックのクローズに成功した場合は OK、それ以外は SYSERR
 */
devcall loopbackClose(device *devptr)
{
    struct loopback *lbkptr;
    irqmask im;

    lbkptr = &looptab[devptr->minor];

    im = disable();
    if (LOOP_STATE_ALLOC != lbkptr->state)
    {
        restore(im);
        return SYSERR;
    }

    /* free the semaphore */
    semfree(lbkptr->sem);
    lbkptr->state = LOOP_STATE_FREE;

    restore(im);
    return OK;
}
