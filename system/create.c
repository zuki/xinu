/**
 * @file create.c
 */
/* Embedded Xinu, Copyright (C) 2007, 2013.  All rights reserved. */

#include <platform.h>
#include <string.h>
#include <thread.h>
#include <core.h>

static int thrnew(void);

/**
 * @ingroup threads
 *
 * プロシージャの実行を開始するためのスレッドを作成する
 *
 * @param procaddr
 *      プロシージアドレス
 * @param ssize
 *      スタックサイズ（バイト単位）
 * @param priority
 *      スレッドの優先度（0は最低の優先度）
 * @param name
 *      スレッド名。デバッグ時に使用
 * @param nargs
 *      引数の数
 * @param ...
 *      スレッドプロシージに渡す引数
 * @return
 *      新規スレッドのid, （メモリ不足やスレッドエントリがフル
 *      などで）新規スレッドが作成できなかった場合は ::SYSERR
 */
tid_typ create(void *procaddr, uint ssize, int priority,
               const char *name, int nargs, ...)
{
    irqmask im;                 /* saved interrupt state               */
    ulong *saddr;               /* stack address                       */
    tid_typ tid;                /* new thread ID                       */
    struct thrent *thrptr;      /* pointer to new thread control block */
    va_list ap;                 /* list of thread arguments            */

    im = disable();

    if (ssize < MINSTK)
    {
        ssize = MINSTK;
    }

    /* 新規スタックを割り当てる  */
    saddr = stkget(ssize);
    if (SYSERR == (int)saddr)
    {
        restore(im);
        return SYSERR;
    }

    /* 新規スレッドIDを割り当てる  */
    tid = thrnew();

    thrtab_acquire(tid);

    if (SYSERR == (int)tid)
    {
        thrtab_release(tid);

        stkfree(saddr, ssize);
        restore(im);
        return SYSERR;
    }

    /* 新規スレッド用のスレッドテーブルエントリを設定する */
    thrcount++;
    thrptr = &thrtab[tid];

    thrptr->state = THRSUSP;
    thrptr->prio = priority;
    thrptr->stkbase = saddr;
    thrptr->stklen = ssize;
    strlcpy(thrptr->name, name, TNMLEN);
    thrptr->parent = gettid();
    thrptr->hasmsg = FALSE;
    thrptr->memlist.next = NULL;
    thrptr->memlist.length = 0;

    /* デフォルトのファイルディスクリプタを設定する  */
    thrptr->fdesc[0] = CONSOLE; /* stdin  is console */
    thrptr->fdesc[1] = CONSOLE; /* stdout is console */
    thrptr->fdesc[2] = CONSOLE; /* stderr is console */

    /* 新規スレッドのスタックをコンテキストレコードと引数で設定する
     * アーキテクチャ固有  */
    va_start(ap, nargs);
    thrptr->stkptr = setupStack(saddr, procaddr, INITRET, nargs, ap);
    va_end(ap);

    thrtab_release(tid);

    /* 割り込み状態を復元して新規スレッドのTIDを返す  */
    restore(im);
    return tid;
}

/*
 * 新規（空き）スレッドIDを取得する。空いているスレッドIDを返す。
 * すべてのスレッドIDがすでに使用されている場合は SYSERR を返す。
 * この関数はIRQは無効であると仮定しているので、スレッドテーブルの
 * 内容は普遍である。
 */
static int thrnew(void)
{
    int tid;
    static int nexttid = 0;

    /* NTHREAD個のスロットをすべてチェックする   */
    for (tid = 0; tid < NTHREAD; tid++)
    {
        nexttid = (nexttid + 1) % NTHREAD;
        if (THRFREE == thrtab[nexttid].state)
        {
            return nexttid;
        }
    }
    return SYSERR;
}

/**
 * thrtabをロックする.
 *
 * @param tid スレッドID
 */
void thrtab_acquire(tid_typ tid)
{
    pldw(&thrtab[tid]);
    pldw(&thrtab[tid].core_affinity);
    mutex_acquire(thrtab_mutex[tid]);
}

/**
 * thrtabのロックを解除する.
 *
 * @param tid スレッドID
 */
void thrtab_release(tid_typ tid)
{
    dmb();
    mutex_release(thrtab_mutex[tid]);
}
