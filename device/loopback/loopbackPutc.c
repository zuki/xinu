/**
 * @file     loopbackPutc.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <loopback.h>
#include <semaphore.h>
#include <interrupt.h>

/**
 * @ingroup loopback
 *
 * ループバックバッファに1文字置く
 *
 * @param devptr
 *      ループバックデバイス
 * @param ch
 *      出力する文字
 *
 * @return
 *      成功した場合は <code>char</code> の @p ch を
 *      @c int にキャストして返す; バッファに空きがない場合は SYSERR を返す。
 */
devcall loopbackPutc(device *devptr, char ch)
{
    struct loopback *lbkptr;
    irqmask im;
    int i;

    lbkptr = &looptab[devptr->minor];

    im = disable();

    /* Ensure room in buffer */
    if (LOOP_BUFFER <= semcount(lbkptr->sem))
    {
        restore(im);
        return SYSERR;
    }

    i = (lbkptr->index + semcount(lbkptr->sem)) % LOOP_BUFFER;
    lbkptr->buffer[i] = (uchar)ch;

    /* signal that more data is on the buffer */
    signal(lbkptr->sem);

    restore(im);

    return (int)ch;
}
