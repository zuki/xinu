/**
 * @file memfree.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <platform.h>
#include <memory.h>
#include <interrupt.h>

/**
 * @ingroup memory_mgmt
 *
 * ヒープに割り当てたメモリブロックを開放する
 *
 * @param memptr
 *      memget()で割り当てたメモリブロックへのポインタ
 *
 * @param nbytes
 *      バイト単位のメモリブロック長（memget()に渡した値と同じ）
 *
 * @return
 *      成功したら ::OK ; 失敗したら ::SYSERR 。この関数は
 *      メモリの破損や間違ったメモリブロックが指定された場合にのみ
 *      失敗する可能性がある。
 */
syscall memfree(void *memptr, uint nbytes)
{
    register struct memblock *block, *next, *prev;
    irqmask im;
    ulong top;

    /* ブロックがヒープにあること */
    if ((0 == nbytes)
        || ((ulong)memptr < (ulong)memheap)
        || ((ulong)memptr > (ulong)platform.maxaddr))
    {
        return SYSERR;
    }

    block = (struct memblock *)memptr;
    nbytes = (ulong)roundmb(nbytes);

    im = disable();

    prev = &memlist;
    next = memlist.next;
    while ((next != NULL) && (next < block))
    {
        prev = next;
        next = next->next;
    }

    /* 前方のmemblockの先頭を探す */
    if (prev == &memlist)
    {
        top = NULL;
    }
    else
    {
        top = (ulong)prev + prev->length;
    }

    /* ブロックが前方または後方のブロックと重ならないこと */
    if ((top > (ulong)block)
        || ((next != NULL) && ((ulong)block + nbytes) > (ulong)next))
    {
        restore(im);
        return SYSERR;
    }

    memlist.length += nbytes;

    /* 前方のブロックと隣接している場合は合体する */
    if (top == (ulong)block)
    {
        prev->length += nbytes;
        block = prev;
    }
    else
    {
        block->next = next;
        block->length = nbytes;
        prev->next = block;
    }

    /* 後方のブロックと隣接している場合は合体する */
    if (((ulong)block + block->length) == (ulong)next)
    {
        block->length += next->length;
        block->next = next->next;
    }
    restore(im);
    return OK;
}
