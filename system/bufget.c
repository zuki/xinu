/**
 * @file bufget.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <semaphore.h>
#include <interrupt.h>
#include <bufpool.h>
#include <string.h>

/**
 * @ingroup memory_mgmt
 *
 * バッファプールからバッファを割り当てる。現在利用可能なバッファが
 * ない場合、この関数はバッファが開いて、スレッドが再スケジュール
 * されるまで待機する。呼び出したコードはバッファの利用が終わったら
 * buffree()でバッファを返さなければならない。
 *
 * @param poolid
 *      バッファプールの識別子（bfpalloc()で返されたID）
 * @param file ファイル名
 * @param func 関数名
 *
 * @return
 *      @p poolid が正しいバッファプールを指定していない場合は
 *      ::SYSErr を返す。そうでなければバッファへのポインタを返す。
 */
void *bufget_(int poolid, const char *file, const char *func)
{
    struct bfpentry *bfpptr;
    struct poolbuf *bufptr;
    irqmask im;

    if (isbadpool(poolid))
    {
        return (void *)SYSERR;
    }

    bfpptr = &bfptab[poolid];

    im = disable();
    if (0 == strncmp("etherWrite", func, 10)) {
        // kprintf("bfpptr->freebuf: %d\n", bfpptr->freebuf);
        // kprintf("bfpptr->freebuf->count: %d\n", semcount(bfpptr->freebuf));
    }
    wait(bfpptr->freebuf);
    bufptr = bfpptr->next;
    bfpptr->next = bufptr->next;
    restore(im);

    bufptr->next = bufptr;
    return (void *)(bufptr + 1);        /* 過去のaccount構造をスキップする+1 */
}
