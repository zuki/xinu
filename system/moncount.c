/**
 * @file moncount.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <monitor.h>

/**
 * @ingroup monitors
 *
 * モニターのカウント（所有するスレッドによりロックされた回数。
 * 現在、モニターを所有するスレッドがない場合は 0）を取得する.
 *
 * この関数はロックをせずに実行する。指定したモニターが潜在的に同時に
 * 解放、ロック、アンロックされる可能性がある場合、呼び出し側は一時的に
 * 割り込みを無効にしなければならない。
 *
 * @param mon
 *      カウントを取得するモニター
 *
 * @return
 *      @p mon に正しい、割り当て済みのモニターが指定された場合、
 *      そのカウントが返される。それ以外は、::SYSERRが返される。
 */
syscall moncount(monitor mon)
{
    if (isbadmon(mon))
    {
        return SYSERR;
    }

    return (montab[mon].count);
}
