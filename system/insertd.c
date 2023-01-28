/**
 * @file insertd.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <thread.h>
#include <queue.h>

/**
 * @ingroup threads
 *
 * デルタキューにスレッドを昇順で挿入する
 * @param tid    挿入するスレッドのid
 * @param q      スレッドを挿入するキュー
 * @param key    デルタキー
 * @return OK
 */
int insertd(tid_typ tid, qid_typ q, int key)
{
    int next;                   /* runs through list                  */
    int prev;                   /* follows next through list          */

    if (isbadqid(q) || isbadtid(tid))
    {
        return SYSERR;
    }

    prev = quehead(q);
    next = quetab[quehead(q)].next;
    while ((quetab[next].key <= key) && (next != quetail(q)))
    {
        key -= quetab[next].key;    // keyをprev.keyとの差分に変換
        prev = next;
        next = quetab[next].next;
    }
    quetab[tid].next = next;
    quetab[tid].prev = prev;
    quetab[tid].key = key;
    quetab[prev].next = tid;
    quetab[next].prev = tid;
    if (next != quetail(q))
    {
        quetab[next].key -= key;    // nextのkeyをtidのkeyとの差分に変換
    }

    return OK;
}
