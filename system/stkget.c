/**
 * @file stkget.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <memory.h>
#include <interrupt.h>
#include <platform.h>

/**
 * @ingroup memory_mgmt
 *
 * スタックメモリを割り当てる
 *
 * @param nbytes
 *      要求されたバイト数
 *
 * @return
 *      @p nbytes が 0 または、要求を満たすだけのメモリがなかった
 *      場合は ::SYSERR; そうでなければ割り当てたメモリ領域の
 *      <b>最上位（最大アドレス）ワード</b>へのポインタを返す。
 *      これはスタックが下に伸びるベースとなることを意図している。
 *      スタックを使い終わったら stkfree() でスタックを解放すること。
 */
void *stkget(uint nbytes)
{
    irqmask im;
    struct memblock *prev, *next, *fits, *fitsprev;

    if (0 == nbytes)
    {
        return (void *)SYSERR;
    }

    /* memblockサイズの倍数に丸める */
    nbytes = (uint)roundmb(nbytes);

    im = disable();

    prev = &memlist;
    next = memlist.next;
    fits = NULL;
    fitsprev = NULL;

    /* 適合する最後尾のブロックをリストからスキャンする */
    while (next != NULL)
    {
        if (next->length >= nbytes)
        {
            fits = next;
            fitsprev = prev;
        }
        prev = next;
        next = next->next;
    }

    if (NULL == fits)
    {
        /* nbytes以上のブロックなし */
        restore(im);
        return (void *)SYSERR;
    }

    if (nbytes == fits->length)
    {
        fitsprev->next = fits->next;
    }
    else
    {
        /* 先頭部分を切り取る（残す） */
        fits->length -= nbytes;
        fits = (struct memblock *)((ulong)fits + fits->length);
    }

    memlist.length -= nbytes;
    restore(im);
    // 取得するのはnbytes以上ある最後尾のブロックの後ろからnbytes
    return (void *)((ulong)fits + nbytes - sizeof(int));
}
