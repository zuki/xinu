/**
 * @file bfpalloc.c
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
 * ヒープストレージを取得し、バッファに細分化する
 *
 * @param bufsize
 *      個々のバッファのサイズ（バイト単位）
 *
 * @param nbuf
 *      プールにあるバッファ数
 *
 * @return
 *      成功の場合、bufget(), bfpfree()にわたすことができる
 *      バッファプールの識別子を返す。失敗の場合は ::SYSERR を返す。
 */
int bfpalloc(uint bufsize, uint nbuf)
{
    struct bfpentry *bfpptr;
    struct poolbuf *bufptr;
    int id, buffer;
    irqmask im;

    bufsize = roundword(bufsize) + sizeof(struct poolbuf);

    if (bufsize > POOL_MAX_BUFSIZE ||
        bufsize < POOL_MIN_BUFSIZE || nbuf > POOL_MAX_NBUFS || nbuf < 1)
    {
        return SYSERR;
    }

    im = disable();
    for (id = 0; id < NPOOL; id++)
    {
        bfpptr = &bfptab[id];
        if (BFPFREE == bfpptr->state)
        {
            break;
        }
    }
    // プールは全て使用中
    if (NPOOL == id)
    {
        restore(im);
        return SYSERR;
    }

    // 空きプールが見つかったので使用する
    bfpptr->state = BFPUSED;
    restore(im);

    bfpptr->freebuf = semcreate(0);     // countは0
    if (SYSERR == (int)bfpptr->freebuf)
    {
        bfpptr->state = BFPFREE;
        return SYSERR;
    }

    bfpptr->nbuf = nbuf;
    bfpptr->bufsize = bufsize;
    bufptr = (struct poolbuf *)memget(nbuf * bufsize);
    if ((void *)SYSERR == bufptr)
    {
        semfree(bfpptr->freebuf);
        bfpptr->state = BFPFREE;
        return SYSERR;
    }
    bfpptr->next = bufptr;
    bfpptr->head = bufptr;
    for (buffer = 0; buffer < nbuf; buffer++)
    {
        bufptr->poolid = id;
        bufptr->next = (struct poolbuf *)((ulong)bufptr + bufsize);
        bufptr = bufptr->next;
    }
    signaln(bfpptr->freebuf, nbuf);

    return id;
}
