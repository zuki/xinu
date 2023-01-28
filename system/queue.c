/**
 * @file queue.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <thread.h>
#include <queue.h>

struct queent quetab[NQENT];    /**< グローバルスレッドキューテーブル */

/**
 * @ingroup threads
 *
 * スレッドをキューの末尾に挿入する
 * @param  tid  エンキューするスレッドID
 * @param  q    対象のキュー
 * @return エンキューされたスレッドのスレッドID
 */
tid_typ enqueue(tid_typ tid, qid_typ q)
{
    int prev, tail;

    if (isbadqid(q) || isbadtid(tid))
    {
        return SYSERR;
    }

    tail = quetail(q);
    prev = quetab[tail].prev;

    quetab[tid].next = tail;
    quetab[tid].prev = prev;
    quetab[prev].next = tid;
    quetab[tail].prev = tid;
    return tid;
}

/**
 * @ingroup threads
 *
 * リストの先頭のスレッドを削除して返す
 * @param  q  対象のキュー
 * @return 削除されたスレッドのスレッドID、または EMPTY
 */
tid_typ dequeue(qid_typ q)
{
    int tid;

    if (isbadqid(q))
    {
        return SYSERR;
    }
    if (isempty(q))
    {
        return EMPTY;
    }

    tid = getfirst(q);
    if (!isbadtid(tid))
    {
        quetab[tid].prev = EMPTY;
        quetab[tid].next = EMPTY;
    }
    return tid;
}
