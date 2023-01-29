/**
 * @file clock.h
 * Definitions relating to the hardware clock and the Xinu clock interrupt.
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#ifndef _CLOCK_H_
#define _CLOCK_H_

#include <queue.h>
#include <stddef.h>

/**
 * @ingroup timer
 *
 * タイマー解像度。Xinuが1秒あたりに割り込みをスケジュールする
 * 最大回数を意味し、1秒あたりのネイティブクロックサイクルの数
 * では @a ない
 */
#define CLKTICKS_PER_SEC  1000

extern volatile ulong clkticks;
extern volatile ulong clktime;
extern qid_typ sleepq;

/* クロック関数プロトタイプ。注: clkupdate() と clkcount() が
 * ここにあるのはその実装がプラットフォーム依存だからである
 */

/**
 * @ingroup timer
 *
 * クロックとスリープキューを初期化する. この関数は起動時に
 * 呼び出される。
 */
void clkinit(void);

/**
 * @ingroup timer
 *
 * 指定したクロックサイクルが経過した後にタイマー割り込みを
 * トリガーするように設定する
 *
 * @param cycles
 *     タイマー割り込みをトリガーするサイクル数. これはネイティブ
 *     サイクルを意味する（その周波数は platform::clkfreq で指定
 *     されている）
 */
void clkupdate(ulong cycles);

/**
 * @ingroup timer
 *
 * 現在のシステムタイマーサイクルを取得する
 *
 * @return
 *	現在のタイマーサイクル数. 一般に、この関数を連続して2回呼び出した
 *  際に返される値の差分のみが意味を持つ。1秒間に経過するサイクル数は、
 *  @ref platform::clkfreq の「プラットフォームにおける値」で指定される
 */
ulong clkcount(void);

/**
 * @ingroup timer
 *
 * タイマー割り込み用の割り込みハンドラ関数.
 * 将来のある時点で発生する新しいタイマー割り込みをスケジュールし、
 * ::clktime と ::clkticks を更新し、スリープ中のスレッドがあれば
 * 起床させ、 そうでなければプロセッサを再スケジュールする。
 */
interrupt clkhandler(void);

/**
 * @ingroup timer
 *
 * 指定のマイクロ秒の間ビジーウェイトする. この関数は絶対に必要な
 * 場合のみ使用すること。通常は sleep() を呼んで、他のスレッドが
 * すぐにプロセッサを使えるようにするべきである。
 *
 * @param us
 *    待機するマイクロ秒数
 */
void udelay(ulong us);

/**
 * @ingroup timer
 *
 * 指定のミリ秒の間ビジーウェイトする. この関数は絶対に必要な
 * 場合のみ使用すること。通常は sleep() を呼んで、他のスレッドが
 * すぐにプロセッサを使えるようにするべきである。
 *
 * @param ms
 *    待機するミリ秒数
 */
void mdelay(ulong ms);

#endif                          /* _CLOCK_H_ */
