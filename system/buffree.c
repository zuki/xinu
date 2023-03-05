/**
 * @file buffree.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <semaphore.h>
#include <interrupt.h>
#include <bufpool.h>

/**
 * @ingroup memory_mgmt
 *
 * バッファをバッファプールに返す
 *
 * @param buffer
 *      解放するバッファのアドレス（bufget()で返されたアドレス）
 *
 * @return
 *      バッファの解放が成功した場合は ::OK; そうでなければ ::SYSERR
 *      ::SYSERRが返されるのはメモリが破損した場合か @p buff に無効な
 *      値が指定された場合だけ。
 */
syscall buffree(void *buffer)
{
    struct bfpentry *bfpptr;
    struct poolbuf *bufptr;
    irqmask im;

    // -1 (bufget()で+1して返されるため)
    bufptr = ((struct poolbuf *)buffer) - 1;

    if (isbadpool(bufptr->poolid))
    {
        return SYSERR;
    }

    if (bufptr->next != bufptr)
    {
        return SYSERR;
    }

    bfpptr = &bfptab[bufptr->poolid];

    im = disable();
    bufptr->next = bfpptr->next;
    bfpptr->next = bufptr;
    restore(im);
    signaln(bfpptr->freebuf, 1);

    return OK;
}
