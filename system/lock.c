/**
 * @file lock.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <monitor.h>

/**
 * @ingroup monitors
 *
 * モニターをロックする.
 *
 * モニターを所有するスレッドがない場合、現在のスレッドを所有者とし、
 * カウントを1に設定する。
 *
 * 現在のスレッドが既にこのモニターを所有している場合、そのカウントを増分して、
 * それ以上の操作は行わない。
 *
 * 他のスレッドがこのモニタを所有している場合、現在のスレッドはそのスレッドが
 * モニタを完全にアンロックするのを待ち、その後、その所有者を現在のスレッドに、
 * そのカウントを1に設定する。
 *
 * @param mon
 *      ロックするモニター
 *
 * @return
 *      成功の場合は、::OK; 失敗の場合（@p mon が正しい、割り当て済みの
 *      モニターでない）は、:SYSERR
 */
syscall lock(monitor mon)
{
    struct monent *monptr;
    irqmask im;

    im = disable();
    if (isbadmon(mon))
    {
        restore(im);
        return SYSERR;
    }

    monptr = &montab[mon];

    /* ロックを所有するスレッドがない場合、現在のスレッドを所有者とする */
    if (NOOWNER == monptr->owner)
    {
        monptr->owner = thrcurrent;     /* current thread now owns the lock  */
        (monptr->count)++;      /* add 1 "lock" to the monitor's count */
        wait(monptr->sem);      /* this thread owns the semaphore      */
    }
    else
    {
        /* 現在のスレッドがロックの所有者の場合はカウントを増分しセマフォを待たない */
        if (thrcurrent == monptr->owner)
        {
            (monptr->count)++;
        }
        /* 別のスレッドが所有者の場合はモニターが開放されるまでセマフォを待つ */
        else
        {
            wait(monptr->sem);
            monptr->owner = thrcurrent;
            (monptr->count)++;

        }
    }

    restore(im);
    return OK;
}
