/**
 * @file insert.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <thread.h>
#include <queue.h>

/**
 * @ingroup threads
 *
 * スレッドをキューにキーの降順に挿入する
 * @param tid    挿入するスレッドID
 * @param q      対象のキュー
 * @param key    ソートキー
 * @return OK
 */
int insert(tid_typ tid, qid_typ q, int key)
{
    int next;                   /* runs through list         */
    int prev;                   /* follows next through list */

    if (isbadqid(q) || isbadtid(tid))
    {
        return SYSERR;
    }

    next = quetab[quehead(q)].next;
    while (quetab[next].key >= key)
    {
        next = quetab[next].next;
    }

    /* prevとnextの間にtidを挿入 */
    quetab[tid].next = next;
    quetab[tid].prev = prev = quetab[next].prev;
    quetab[tid].key = key;
    quetab[prev].next = tid;
    quetab[next].prev = tid;
    return OK;
}
