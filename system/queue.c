/**
 * @file queue.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <thread.h>
#include <queue.h>
#include <core.h>

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

    quetab_acquire();

    tail = quetail(q);
    prev = quetab[tail].prev;

    quetab[tid].next = tail;
    quetab[tid].prev = prev;
    quetab[prev].next = tid;
    quetab[tail].prev = tid;

    quetab_release();

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

    quetab_acquire();
    if (!isbadtid(tid))
    {
        quetab[tid].prev = EMPTY;
        quetab[tid].next = EMPTY;
    }
    quetab_release();

    return tid;
}

void quetab_acquire()
{
    for (int i = 0; i < NQENT; i++)
    {
        pldw(&quetab[i]);
    }
    mutex_acquire(quetab_mutex);
}

void quetab_release()
{
    dmb();
    mutex_release(quetab_mutex);
}
