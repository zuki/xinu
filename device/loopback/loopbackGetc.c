/**
 * @file     loopbackGetc.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <semaphore.h>
#include <loopback.h>
#include <stdio.h>
#include <interrupt.h>

/**
 * @ingroup loopback
 *
 * ループバックバッファから1文字取得する、ブロックされる可能性がある.
 *
 * @param devptr
 *      ループバックデバイスへのポインタ
 *
 * @return
 *      成功した場合は、<code>unsigned char</code> として得られた @p ch を
 *      @c int にキャストして返す。データがなく、デバイスがノンブロッキング
 *      モードの場合は @c EOF を返す。
 */
devcall loopbackGetc(device *devptr)
{
    irqmask im;
    struct loopback *lbkptr;
    uchar ch;

    lbkptr = &looptab[devptr->minor];

    im = disable();

    /* wait until the buffer has data */
    if (LOOP_NONBLOCK == (lbkptr->flags & LOOP_NONBLOCK)) {
        if (semcount(lbkptr->sem) <= 0) {
            restore(im);
            return EOF;
        }
    }

    wait(lbkptr->sem);

    /* Get and return the next character.  */
    ch = lbkptr->buffer[lbkptr->index];
    lbkptr->index = (lbkptr->index + 1) % LOOP_BUFFER;
    restore(im);
    return ch;
}
