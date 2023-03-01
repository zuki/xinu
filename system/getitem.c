/**
 * @file getitem.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <queue.h>

/**
 * @ingroup threads
 *
 * キューの先頭からスレッドを削除する
 * @param  q  対象のキュー
 * @return 削除したスレッドのスレッドID
 */
tid_typ getfirst(qid_typ q)
{
    tid_typ head;

    if (isbadqid(q))
    {
        return SYSERR;
    }
    if (isempty(q))
    {
        return EMPTY;
    }

    head = quehead(q);
    return getitem(quetab[head].next);
}

/**
 * @ingroup threads
 *
 * キューの末尾からスレッドを削除する
 * @param  q  対象のキュー
 * @return 削除したスレッドのスレッドID
 */
tid_typ getlast(qid_typ q)
{
    tid_typ tail;

    if (isbadqid(q))
    {
        return SYSERR;
    }
    if (isempty(q))
    {
        return EMPTY;
    }

    tail = quetail(q);
    return getitem(quetab[tail].prev);
}

/**
 * @ingroup threads
 *
 * キューの任意の場所からスレッドを削除する
 * @param  tid  削除するスレッドID
 * @return 削除したスレッドのスレッドID
 */
tid_typ getitem(tid_typ tid)
{
    tid_typ prev, next;

    quetab_acquire();

    next = quetab[tid].next;
    prev = quetab[tid].prev;
    quetab[prev].next = next;
    quetab[next].prev = prev;
    quetab[tid].next = EMPTY;
    quetab[tid].prev = EMPTY;

    quetab_release();

    return tid;
}
