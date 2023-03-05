/**
 * @file bfpfree.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <interrupt.h>
#include <memory.h>
#include <bufpool.h>

/**
 * @ingroup memory_mgmt
 *
 * バッファプールに割り当てられたメモリを解放する
 *
 * @param poolid
 *      解放するバッファプールの識別子（bpfalloc()で返されたID）
 *
 * @return
 *      バッファプールが有効で解放に成功したバアは ::OK;
 *      そうでない場合は ::SYSERR。
 *      @p poolid に正しいバッファプールが指定された場合、この関数が
 *      ::SYSERR を返すのはメモリが破損されていた場合のだけ。
 */
syscall bfpfree(int poolid)
{
    struct bfpentry *bfpptr;
    irqmask im;

    if (isbadpool(poolid))
    {
        return SYSERR;
    }

    bfpptr = &bfptab[poolid];

    im = disable();
    bfpptr->state = BFPFREE;
    if (SYSERR == memfree(bfpptr->head, bfpptr->nbuf * bfpptr->bufsize))
    {
        restore(im);
        return SYSERR;
    }
    if (SYSERR == semfree(bfpptr->freebuf))
    {
        restore(im);
        return SYSERR;
    }
    restore(im);

    return OK;
}
