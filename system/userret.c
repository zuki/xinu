/**
 * @file userret.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>

/**
 * @ingroup threads
 *
 * スレッドがreturnで終了するときに呼ばれる
 */
void userret(void)
{
    kill(gettid());
}
