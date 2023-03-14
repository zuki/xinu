/**
 * @file moncreate.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <monitor.h>

static monitor monalloc(void);

/**
 * @ingroup monitors
 *
 * 新しいモニターを作成して初期化する.
 *
 * @return
 *      成功の場合は、新しいモニターを返す。失敗の場合（システムのモニター、
 *      またはセマフォが枯渇）は ::SYSERR を返す。
 */
monitor moncreate(void)
{
    irqmask im;
    monitor mon;
    struct monent *monptr;

    /* Disable interrupts.  */
    im = disable();

    /* モニターテーブルのエントリを割り当てる  */
    mon = monalloc();

    if (SYSERR != mon)
    {
        monptr = &montab[mon];

        /* モニターは初期には所有者がなく、カウントは0  */
        monptr->owner = NOOWNER;
        monptr->count = 0;

        /* モニターのセマフォをカウント 1 で初期化し、1つのスレッドが
         * モニターを取得できるようにする  */
        monptr->sem = semcreate(1);
        if (SYSERR == monptr->sem)
        {
            monptr->state = MFREE;
            mon = SYSERR;
        }
    }

    /* Restore interrupts.  */
    restore(im);

    /* モニターテーブルのインデックス、またはSYSERRを返す  */
    return mon;
}

/* 未使用のモニターテーブルエントリのインデックスを返す。
 * または、未使用のエントリがなかった場合は、SYSERRを返す。*/
static monitor monalloc(void)
{
#if NMON
    int i;
    static int nextmon = 0;

    /* Check all NMON slots, starting at 1 past the last slot searched.  */
    for (i = 0; i < NMON; i++)
    {
        nextmon = (nextmon + 1) % NMON;
        if (MFREE == montab[nextmon].state)
        {
            montab[nextmon].state = MUSED;
            return nextmon;
        }
    }
#endif
    return SYSERR;
}
