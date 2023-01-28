/**
 * @file gettid.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>

/**
 * @ingroup threads
 *
 * 現在実行しているスレッドのスレッドIDを取得する
 * @return 現在実行しているスレッドのスレッドID
 */
tid_typ gettid(void)
{
    return thrcurrent;
}
