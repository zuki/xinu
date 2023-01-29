/**
 * @file semaphore.h
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include <queue.h>

/* セマフォの状態定義 */
#define SFREE 0x01 /**< このセマフォは空き */
#define SUSED 0x02 /**< このセマフォは使用中 */

/* "semaphore"のタイプ定義 */
typedef unsigned int semaphore;

/**
 * セマフォテーブルエントリ
 */
struct sement                   /* セマフォテーブルエントリ     */
{
    char state;                 /**< 状態（SFREE か SUSED） */
    int count;                  /**< このセマフォのカウント */
    qid_typ queue;              /**< このセマフォで待機しているスレッドのキュー: queue.h が必要 */
};

extern struct sement semtab[];

/* isbadsem - 指定されたセマフォIDと状態をチェックする */
#define isbadsem(s) ((s >= NSEM) || (SFREE == semtab[s].state))

/* セマフォ関数プロトタイプ */
syscall wait(semaphore);
syscall signal(semaphore);
syscall signaln(semaphore, int);
semaphore semcreate(int);
syscall semfree(semaphore);
syscall semcount(semaphore);

#endif                          /* _SEMAPHORE_H */
