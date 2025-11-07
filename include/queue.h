/**
 * @file queue.h
 *
 * スレッドキューシステムは、動的メモリ割り当てのようなより複雑な
 * OSサービスが稼働する前に、静的に割り当てられた配列を用いてソート
 * 済みスレッドキューをモデル化することを可能にする。
 * 
 * これらのスレッドキューはいくつかの重要な不変条件を前提とする.
 *   1) コンパイル時に既知の、システム内の固定数のスレッドキュー
 *   2) スレッドキューにはスレッドとその関連キー値のみが含まれる。
 *      そのため、キュー長は最大でNTHREAD（スレッド総数）となる。
 *   3) スレッドは同時に最大1つのキューにしか存在できない。
 * 
 * 特定のキュー内のスレッド順序は、そのシステムキューを維持する際に
 * 呼び出されるソート関数に依存する。
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

/* queue structure declarations, constants, and inline procedures       */

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <kernel.h>
#include <mutex.h>

#ifndef NQENT

/** NQENT = 1 per thread, 2 per list, 2 per sem */
#define NQENT   (NTHREAD + 4 + 6 + NSEM + NSEM)
#endif

#define EMPTY (-2)              /**< null pointer for queues            */
#define MAXKEY 0x7FFFFFFF       /**< max key that can be saved in queue */
#define MINKEY 0x80000000       /**< min key that can be saved in queue */

typedef int qid_typ;            /**< represent queue id by head index   */

/**
 * Defines what an entry in the queue table looks like.
 */
struct queent
{
    int key;                    /**< key on which the queue is ordered  */
    tid_typ next;               /**< index of next thread or tail       */
    tid_typ prev;               /**< index of previous thread or head   */
};

extern struct queent quetab[];
extern qid_typ readylist[];

extern mutex_t quetab_mutex;
void quetab_acquire(void);
void quetab_release(void);

#define quehead(q) (q)
#define quetail(q) ((q) + 1)

/* Check for invalid queue ids.  Note that interrupts must be disabled  */
/* for the condition to hold true between statements.                   */
#define isbadqid(x)  ((quehead(x) < 0) || \
                      (quehead(x) != (quetail(x) - 1)) || \
                      (quetail(x) >= NQENT))

#define isempty(q)   (quetab[quehead(q)].next >= NTHREAD)
#define nonempty(q)  (quetab[quehead(q)].next <  NTHREAD)
#define firstkey(q)  (quetab[quetab[quehead(q)].next].key)
#define lastkey(q)   (quetab[quetab[quetail(q)].prev].key)
#define firstid(q)   (quetab[quehead(q)].next)

/* Queue function prototypes */
tid_typ getfirst(qid_typ);
tid_typ getlast(qid_typ);
tid_typ getitem(tid_typ);
tid_typ enqueue(tid_typ, qid_typ);
tid_typ dequeue(qid_typ);
int insert(tid_typ, qid_typ, int);
int insertd(tid_typ, qid_typ, int);
qid_typ queinit(void);

#endif                          /* _QUEUE_H_ */
