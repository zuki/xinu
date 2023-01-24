/**
 * @file malloc.c
 * ユーザスレッドにメモリを割り当てる
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <memory.h>
#include <safemem.h>
#include <interrupt.h>
#include <thread.h>
#include <stdlib.h>

/**
 * ヒープストレージを要求し、会計情報を記録し、割り当てられた
 * メモリ領域へのポインタを返す。
 * @param nbytes 要求するバイト数
 * @return 成功の場合は領域へのポインタ、失敗の場合は SYSERR
 */
void *malloc(uint nbytes)
{
#ifdef UHEAP_SIZE
    irqmask im;
    struct thrent *thread;
    struct memregion *region;
    struct memblock *prev, *curr, *leftover;

    /* 0 バイトは割り当てない */
    if (0 == nbytes)
    {
        return NULL;
    }

    /* memblockサイズの倍数に丸める */
    nbytes = (uint)roundmb(nbytes);

    /* 会計情報用のスペースを作る */
    nbytes += sizeof(struct memblock);

    /* スレッドポインタをセットする */
    thread = &thrtab[thrcurrent];

    im = disable();

    prev = &(thread->memlist);
    curr = thread->memlist.next;
    while (curr != NULL)
    {
        if (curr->length == nbytes)
        {
            prev->next = curr->next;
            thread->memlist.length -= nbytes;

            break;
        }
        else if (curr->length > nbytes)
        {
            /* ブロックを2つに分割する */
            leftover = (struct memblock *)((ulong)curr + nbytes);
            prev->next = leftover;
            leftover->next = curr->next;
            leftover->length = curr->length - nbytes;
            thread->memlist.length -= nbytes;

            break;
        }
        prev = curr;
        curr = curr->next;
    }

    /* スレッドメモリに割り当てができたか? */
    if (curr != NULL)
    {
        /* 会計情報をセット */
        curr->next = curr;
        curr->length = nbytes;

        restore(im);
        return (void *)(curr + 1);  // 会計情報は見せない
    }

    /* カーネルからメモリを取得する */
    region = memRegionAlloc(nbytes);
    if (SYSERR == (int)region)
    {
        restore(im);
        return NULL;
    }

    /* カレントポインタをセットする */
    curr = region->start;

    /* 必要以上のメモリを取得した場合は残りのメモリを切り取る */
    if (region->length > nbytes)
    {
        leftover = (struct memblock *)((uint)region->start + nbytes);
        leftover->next = leftover;
        leftover->length = region->length - nbytes;

        /* 余分なメモリをfreelistに置く */
        free(++leftover);
    }

    /* 会計情報をセットする */
    curr->next = curr;
    curr->length = nbytes;

    /* メモリをシステムページテーブルにマップする */
    safeMapRange(curr, nbytes, ENT_USER);

    restore(im);
    return (void *)(curr + 1);
#else
    return NULL;
#endif                          /* UHEAP_SIZE */
}
