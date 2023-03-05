/**
 * @file memget.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <interrupt.h>
#include <memory.h>

/**
 * @ingroup memory_mgmt
 *
 * ヒープメモリを割り当てる
 *
 * @param nbytes
 *      要求するバイト数
 *
 * @return
 *      @p nbytes が 0、または要求に答え荒れるメモリが場合は ::SYSERR;
 *      そうでなければ割り当てたメモリ領域へのポインタを返す。
 *      返されるポインタは8バイト境界にあることが保証される。
 *      使用が終わったら memfree() でブロックを解放すること。
 */
void *memget(uint nbytes)
{
    register struct memblock *prev, *curr, *leftover;
    irqmask im;

    if (0 == nbytes)
    {
        return (void *)SYSERR;
    }

    /* memblockサイズの倍数に丸める   */
    nbytes = (ulong)roundmb(nbytes);

    /* 割り込みを禁止する */
    im = disable();

    prev = &memlist;
    curr = memlist.next;
    while (curr != NULL)
    {
        if (curr->length == nbytes)
        {
            prev->next = curr->next;
            memlist.length -= nbytes;

            restore(im);
            return (void *)(curr);
        }
        else if (curr->length > nbytes)
        {
            /* ブロックを2つに分割する */
            leftover = (struct memblock *)((ulong)curr + nbytes);
            prev->next = leftover;
            leftover->next = curr->next;
            leftover->length = curr->length - nbytes;
            memlist.length -= nbytes;

            restore(im);
            return (void *)(curr);
        }
        prev = curr;
        curr = curr->next;
    }
    restore(im);
    return (void *)SYSERR;
}
