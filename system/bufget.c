/**
 * @file bufget.c
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
 * バッファプールからバッファを割り当てる。現在利用可能なバッファが
 * ない場合、この関数はバッファが開いて、スレッドが再スケジュール
 * されるまで待機する。呼び出したコードはバッファの利用が終わったら
 * buffree()でバッファを返さなければならない。
 *
 * @param poolid
 *      バッファプールの識別子（bfpalloc()で返されたID）
 *
 * @return
 *      @p poolid が正しいバッファプールを指定していない場合は
 *      ::SYSErr を返す。そうでなければバッファへのポインタを返す。
 */
void *bufget(int poolid)
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
    wait(bfpptr->freebuf);
    bufptr = bfpptr->next;
    bfpptr->next = bufptr->next;
    restore(im);

    bufptr->next = bufptr;
    return (void *)(bufptr + 1);        /* 過去のaccount構造をスキップする+1 */
}
