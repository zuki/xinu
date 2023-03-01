/**
 * @file queinit.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <queue.h>

/**
 * @ingroup threads
 *
 * グローバルスレッドキューテーブルの新しいキューを初期化する
 * @return 新しく割り当てたキューのID、または SYSERR
 */
qid_typ queinit(void)
{
    static int nextqid = NTHREAD;   /**< 次に利用可能なquetabエントリ */
    qid_typ q;

    if (nextqid > NQENT)
    {
        return SYSERR;
    }

    quetab_acquire();

    q = nextqid;
    nextqid += 2;
    quetab[quehead(q)].next = quetail(q);
    quetab[quehead(q)].prev = EMPTY;
    quetab[quehead(q)].key = MAXKEY;
    quetab[quetail(q)].next = EMPTY;
    quetab[quetail(q)].prev = quehead(q);
    quetab[quetail(q)].key = MINKEY;

    quetab_release();

    return q;
}
