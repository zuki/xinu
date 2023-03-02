/**
 * @file udelay.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <conf.h>

#if RTCLOCK

#include <clock.h>
#include <platform.h>

/* 以下の udelay() の実装はプラットフォームには依存せず、プラット
 * フォーム固有のコードで実装されている clkcount() のみに依存している。
 * ただし、プラットフォームのクロック周波数が 1,000,000 の偶数倍で
 * あることを仮定している。
 */

/**
 * @ingroup timer
 *
 * 指定のマイクロ秒の間ビジーウェイトする。この関数は絶対に必要な
 * 場合のみ使用すること。通常は sleep() を呼んで、他のスレッドが
 * すぐにプロセッサを使えるようにするべきである。
 *
 * @param us
 *    待機するマイクロ秒数
 */
void udelay(ulong us)
{
    /* delay = 待機するタイマーティック数  */
    /* TODO: 除数に1000000を設定するとtestsuiteがうごかない */
    ulong delay = (platform.clkfreq / 1000) * us;

    /* start = 開始時のティックカウント  */
    ulong start = clkcount();

    /* end = 終了時のティックカウント（ラップする可能性あり）  */
    ulong target = start + delay;

    /* 一時変数 */
    ulong count;

    if (target >= start)
    {
        /* 通常のケース:  ティックカウントがtargetより大きくなる、
         * あるいは、ラップしてstartより小さくなるまで待機する
         */
        while (((count = clkcount()) < target) && (count >= start))
            ;
    }
    else
    {
        /* ラップケース:  ティックカウントがラップし、ティック
         * カウントがtargetに到達するまで待機する
         */
        while ((count = clkcount()) > start)
            ;
        while ((count = clkcount()) < target)
            ;
    }
}

#endif /* RTCLOCK */
