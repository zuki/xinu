/**
 * @file interrupt.h
 *
 * 割り込み処理に関する定数と定義.
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include <stddef.h>

typedef interrupt (*interrupt_handler_t)(void);

extern interrupt_handler_t interruptVector[];

typedef unsigned long irqmask;  /**< 無効/復元用のマシン状態  */

/**
 * @ingroup bcm2837
 * 割り込みをグローバルに有効化する.
 * intuitls.Sで実装されている。
 */

void enable(void);
/**
 * @ingroup bcm2837
 * 割り込みをグローバルに無効化して古い状態を返す.
 * intuitls.Sで実装されている。
 * @return 無効にする前の割り込みの状態
 */
irqmask disable(void);

/**
 * @ingroup bcm2837
 * グローバル割り込みマスクを以前の状態に復元する.
 * intuitls.Sで実装されている。
 * @param im 復元する割り込み状態の割り込みマスク
 * @return 呼び出された際の割り込みの状態
 */
irqmask restore(irqmask im);

void enable_irq(irqmask);
void disable_irq(irqmask);

/* IRQ定義をインクルード  */
#include "bcm2837.h"

#endif /* _INTERRUPT_H_ */
