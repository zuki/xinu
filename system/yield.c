/**
 * @file yield.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>

/**
 * @ingroup threads
 *
 * プロセッサを明け渡す
 * @return スレッドがコンテキストスイッチから帰ったら OK
 */
syscall yield(void)
{
    irqmask im;

    im = disable();
    resched();
    restore(im);
    return OK;
}
