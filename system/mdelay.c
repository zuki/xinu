/**
 * @file mdelay.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <conf.h>

#if RTCLOCK

#include <clock.h>

/**
 * @ingroup timer
 *
 * 指定のミリ秒の間ビジーウェイトする。この関数は絶対に必要な
 * 場合のみ使用すること。通常は sleep() を呼んで、他のスレッドが
 * すぐにプロセッサを使えるようにするべきである。
 *
 * @param ms
 *    待機するミリ秒数
 */
void mdelay(ulong ms)
{
    while (ms--)
    {
        udelay(1000);
    }
}

#endif /* RTCLOCK */
