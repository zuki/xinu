/**
 * @file monfree.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <monitor.h>

/**
 * @ingroup monitors
 *
 * moncreate()で割り当てられたモニターを開放する.
 *
 * モニターはロックしているスレッドがなくなった場合にのみ
 * 開放されなくてはならない。すなわち、モニターが所有されていない、
 * killされたスレッドにより所有されているのいずれかである。
 *
 * @param mon
 *      開放するモニター
 *
 * @return
 *      成功の場合は、::OK; 失敗の場合（@p mon が正しい、割り当て済みの
 *      モニターでない）は、:SYSERR
 */
syscall monfree(monitor mon)
{
    struct monent *monptr;
    irqmask im;

    im = disable();

    /* make sure the specified monitor is valid and allocated  */
    if (isbadmon(mon))
    {
        restore(im);
        return SYSERR;
    }

    monptr = &montab[mon];

    /* モニターのセマフォを開放して、モニターテーブルエントリを空きとマークする  */
    semfree(monptr->sem);
    monptr->state = MFREE;

    restore(im);
    return OK;
}
