/**
 * @file monitor.h
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#ifndef _MONITOR_H_
#define _MONITOR_H_

#include <thread.h>
#include <semaphore.h>

#ifndef NMON
#  define NMON 0
#endif

/* モニターの状態定義 */
#define MFREE 0x01 /**< このモニターは空き */
#define MUSED 0x02 /**< このモニターは使用済み */

#define NOOWNER BADTID /**< このモニターのロックを所有しているスレッドはない */

/** "monitor"のタイプ定義 */
typedef unsigned int monitor;

/**
 * モニターテーブルエントリ
 */
struct monent
{
    char state;       /**< モニターの状態（MFREE か MUSED）  */
    tid_typ owner;    /**< ロックを所有しているスレッド、所有者がない場合は NOOWNER */
    uint count;       /**< 実行されたlockアクション数 */
    semaphore sem;    /**< このモニターが使用するセマフォ  */
};

extern struct monent montab[];

/**モニターが不正または未使用かチェックする  */
#define isbadmon(m) ((m >= NMON) || (MFREE == montab[m].state))

/* モニター関数プロトタイプ */
syscall lock(monitor);
syscall unlock(monitor);
monitor moncreate(void);
syscall monfree(monitor);
syscall moncount(monitor);

#endif /* _MONITOR_H */
