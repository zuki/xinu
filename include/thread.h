/**
 * @file thread.h
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _THREAD_H_
#define _THREAD_H_

#ifndef __ASSEMBLER__
#include <interrupt.h>
#include <semaphore.h>
#include <debug.h>
#include <stddef.h>
#include <memory.h>
#endif /* __ASSEMBLER__ */

/* スレッドスタックのトップをマークするありえない値                     */
#define STACKMAGIC  0x0A0AAAA9

/* スレッド状態定位数                                                   */
#define THRCURR     1           /**< スレッドは現在実行中               */
#define THRFREE     2           /**< スレッドスロットは空いている       */
#define THRREADY    3           /**< スレッドはreadyキューにある        */
#define THRRECV     4           /**< スレッドはメッセージを待機中       */
#define THRSLEEP    5           /**< スレッドはスリープ中               */
#define THRSUSP     6           /**< スレッドはサスペンド中             */
#define THRWAIT     7           /**< スレッドはsemaphoreキューにある    */
#define THRTMOUT    8           /**< スレッドはタイムアウト付きで受信中 */
#define THRMIGRATE  9           /**< スレッドはmigrate中                */

/* 様々なスレッド定義                                                   */
#define TNMLEN      16          /**< スレッド"名"の長さ                 */
#define NULLTHREAD  0           /**< nullスレッドのID                   */
#define BADTID      (-1)        /**< 不正なtidが必要な場合に使用する    */

/* スレッド初夏家庭数 */
#ifndef INITSTK
#define INITSTK     65536       /**< 初期スレッドスタックサイズ         */
#endif
#define INITPRIO    20          /**< 初期スレッド優先度                 */
#define MINSTK      128         /**< 最小スレッドスタックサイズ         */
#ifdef JTAG_DEBUG
#define INITRET   debugret      /**< デバッグ用のスレッド復帰アドレス   */
#else                           /* not JTAG_DEBUG */
#define INITRET   userret       /**< スレッド復帰アドレス               */
#endif                          /* JTAG_DEBUG */

/* ready用の再スケジュール定数 */
#define RESCHED_YES 1           /**< readyに再スケジュールを通知        */
#define RESCHED_NO  0           /**< readyに再スケジュールしないよう通知*/

/* 不正なスレッドIDをチェックする。ステートメント間でtrueを保持する     */
/* ための条件のために割り込みは無効でなければならないことに注意         */
#define isbadtid(x) ((x)>=NTHREAD || (x)<0 || THRFREE == thrtab[(x)].state)

/** 1スレッドが保持できるファイルディスクリプタの最大数 */
#define NDESC       5

/** ローカルデバイスの最大数 */
#define NLOCDEV     10

/* アセンブリファイルに公開する sizeof(struct thrent) と */
/* offsetof(struct thrent, stkdiv)  */
#define THRENTSIZE 148
#define STKDIVOFFSET 104

#ifndef __ASSEMBLER__

/**
 * スレッドテーブルエントリが何であるかを定義する
 */
struct thrent
{
    uchar state;                /**< スレッドの状態: THRCURR など       */
    int prio;                   /**< スレッドの優先度                   */
    void *stkptr;               /**< 保存されたスタックポインタ         */
    void *stkbase;              /**< ランタイムスタックベース           */
    ulong stklen;               /**< スタック長（バイト単位）           */
    char name[TNMLEN];          /**< スレッド名                         */
    irqmask intmask;            /**< 保存された割り込みマスク           */
    semaphore sem;              /**< 待機しているセマフォ               */
    tid_typ parent;             /**< 親スレッドのtid                    */
    message msg;                /**< このスレッドへ送られたメッセージ   */
    bool hasmsg;                /**< msgが有効な場合、非ゼロ            */
    struct memblock memlist;    /**< スレッドの空きメモリリスト         */
    int fdesc[NDESC];           /**< スレッドのデバイスディスクリプタ   */
};

extern struct thrent thrtab[];
extern int thrcount;            /**< 現在アクティブなスレッド           */
extern tid_typ thrcurrent;      /**< 現在実行中のスレッド               */

/* スレッド間コミュニケーションのプロトタイプ */
syscall send(tid_typ, message);
message receive(void);
message recvclr(void);
message recvtime(int);

/* スレッド管理関数のプロトタイプ */

tid_typ create(void *procaddr, uint ssize, int priority,
               const char *name, int nargs, ...);
tid_typ gettid(void);
syscall getprio(tid_typ);
syscall kill(int);
int ready(tid_typ, bool);
int resched(void);
syscall sleep(uint);
syscall unsleep(tid_typ);
syscall yield(void);

/**
 * @ingroup threads
 *
 * 割り込みが検知されるまで実行を一時中断 (suspend) する
 * （もしあるなら）何らかのパワーダウン状態に入る
 */
void pause(void);

void userret(void);

#endif /* __ASSEMBLER__ */

#endif                          /* _THREAD_H_ */
