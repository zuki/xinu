/**
 * @file unlock.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <monitor.h>

/**
 * @ingroup monitors
 *
 * モニターをアンロックする.
 *
 * モニタのロックカウント（所有するスレッドがモニタをロックした
 * 回数を示す）を減ずる。その結果、カウントがゼロより大きいままで
 * あれば、それ以上のアクションは行わない。カウントが0になった場合、
 * モニターは所有者なしに設定され、モニターのlock()を待っている
 * 可能性のある最大1つのスレッドを起床させる。
 *
 * これは通常、lock()を行った同じスレッドが引き続いて呼び出すはずの
 * ものであるが、killされたスレッドが所有するモニターのロックを完全に
 * アンロックするために moncount(mon) 回呼び出される場合もある。
 *
 * @param mon
 *      アンロックするモニター
 *
 * @return
 *      成功の場合は、::OK; 失敗の場合（@p mon が正しい、割り当て済みの
 *      モニターであり、ロックカウントが非0でない）は、:SYSERR
 */
syscall unlock(monitor mon)
{
    register struct monent *monptr;
    irqmask im;

    im = disable();
    if (isbadmon(mon))
    {
        restore(im);
        return SYSERR;
    }

    monptr = &montab[mon];

    /* 安全性チェック: モニターは少なくとも1回はロックされていなければならない */
    if (monptr->count == 0)
    {
        restore(im);
        return SYSERR;
    }

    /* "アンロック"を意味するモニターのカウントを1つ減らす */
    (monptr->count)--;

    /* これがトップレベルのアンロック呼び出しの場合、このモニターのロックを開放する */
    if (monptr->count == 0)
    {
        monptr->owner = NOOWNER;
        signal(monptr->sem);
    }

    restore(im);
    return OK;
}
