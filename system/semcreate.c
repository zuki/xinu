/**
 * @file semcreate.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <semaphore.h>
#include <interrupt.h>
#include <core.h>

static semaphore semalloc(void);

/**
 * @ingroup semaphores
 *
 * 初期値として指定されたカウントを持つセマフォを作成する
 *
 * @param count
 *      セマフォの初期カウント（通常、利用可能な何らかのリソースの数）
 *      非負数でなければならない。
 *
 * @return
 *      成功の場合、新しいセマフォを返す。そうでない場合 ::SYSERR を返す。
 *      新しいセマフォは不要になったら、semfree()で解放されなければならない。
 *      この関数はシステムのセマフォが枯渇した場合、あるいは、 @p count が
 *      縁の場合のみ失敗する。
 */
semaphore semcreate(int count)
{
    semaphore sem;
    irqmask im;

    if (count < 0)          /* 負値からはスタートできない  */
    {
        return SYSERR;
    }

    im = disable();         /* Disable interrupts.  */
    sem = semalloc();       /* Allocate semaphore.  */
    if (SYSERR != sem)      /* If semaphore was allocated, set count.  */
    {
        semtab_acquire(sem);
        semtab[sem].count = count;
        semtab_release(sem);
    }
    /* Restore interrupts and return either the semaphore or SYSERR.  */
    restore(im);
    return sem;
}

/**
 * 未使用のセマフォを割り当て、そのIDを返す。
 * 空きエントリを探してグローバルセマフォテーブルを走査し、
 * 見つかったエントリに使用中のマークを付け、新しいセマフォを
 * 返す。
 * @return 成功の場合は利用可能なセマフォID、失敗の場合は SYSERR
 */
static semaphore semalloc(void)
{
    int i;
    static int nextsem = 0;

    /* check all NSEM slots, starting at 1 past the last slot searched.  */
    for (i = 0; i < NSEM; i++)
    {
        nextsem = (nextsem + 1) % NSEM;
        if (SFREE == semtab[nextsem].state)
        {
            semtab_acquire(nextsem);
            semtab[nextsem].state = SUSED;
            semtab_release(nextsem);
            return nextsem;
        }
    }
    return SYSERR;
}


/**
 * semtabロックを取得する.
 *
 * @param sem セマフォ
 */
void semtab_acquire(semaphore sem)
{
    pldw(&semtab[sem]);
    mutex_acquire(semtab_mutex[sem]);
}

/**
 * semtabロックを解放する.
 *
 * @param sem セマフォ
 */
void semtab_release(semaphore sem)
{
    dmb();
    mutex_release(semtab_mutex[sem]);
}
