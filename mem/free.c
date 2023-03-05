/**
 * @file free.c
 * メモリをユーザスレッドから解放する
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <memory.h>
#include <safemem.h>
#include <interrupt.h>
#include <thread.h>

/**
 * 2ワード前に格納されているmalloc()の会計情報に基づいて
 * メモリブロックの解放を試みる。
 * @param *base メモリブロックへのポインタ
 */
void free(void *base)
{
#ifdef UHEAP_SIZE
    struct thrent *thread;
    struct memblock *block, *next, *prev;
    irqmask im;
    uint top;


    /* baseは解放するmemblockを指している */
    block = (struct memblock *)base;

    /* 会計情報までポインタを戻す */
    block--;

    /* ブロックの基本的なチェックを行う */
    if (block->next != block)
    {
        return;
    }

    /* システムのページテーブルからページをアンマップする */
    safeUnmapRange(block, block->length);

    im = disable();

    /* 現在のスレッドへのポイントを取得する */
    thread = &thrtab[thrcurrent];

    prev = &(thread->memlist);
    next = thread->memlist.next;
    while ((next != NULL) && (next < block))
    {
        prev = next;
        next = next->next;
    }

    /* 前方のmemblockの先頭を探す */
    if (prev == &(thread->memlist))
    {
        top = NULL;
    }
    else
    {
        top = (ulong)prev + prev->length;
    }

    /* 前方または後方のブロックと重ならないこと */
    if ((top > (ulong)block)
        || ((next != NULL)
            && ((ulong)block + block->length) > (ulong)next))
    {
        restore(im);
        return;
    }

    thread->memlist.length += block->length;

    /* 前方ブロックが隣接する場合は合体する */
    if (top == (ulong)block)
    {
        prev->length += block->length;
        block = prev;
    }
    else
    {
        block->next = next;
        prev->next = block;
    }

    /* 後方ブロックが隣接する場合は合体する */
    if (((ulong)block + block->length) == (ulong)next)
    {
        block->length += next->length;
        block->next = next->next;
    }

    restore(im);
#endif                          /* UHEAP_SIZE */
}
