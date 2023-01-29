/**
 * @file semcount.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <semaphore.h>

/**
 * @ingroup semaphores
 *
 * セマフォのカウント値を取得する
 *
 * @param sem
 *      カウント値を取得するセマフォ
 *
 * @return
 *      成功の場合、セマフォのカウント値を返す。そうでない場合は
 *      ::SYSERR を返す。この関数は @p sem に正しいセマフォが指定
 *      されなかった場合にだけ失敗する。
 */
syscall semcount(semaphore sem)
{
    if (isbadsem(sem))
    {
        return SYSERR;
    }

    return (semtab[sem].count);
}
