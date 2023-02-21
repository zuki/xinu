/**
 * @file intutils.S
 *
 * ARM上のグルーバル割り込みを有効化/無効化/復元する関数.
 *
 * 詳細は https://xinu.cs.mu.edu/index.php/Interrupt_handling_(ARM) を参照.
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

.globl enable
.globl disable
.globl restore

/**
 * @fn void enable(void)
 *
 * 割り込みをグローバルに有効化する.
 */
enable:
    .func enable
    cpsie i
    mov pc, lr
    .endfunc

/**
 * @fn irqmask disable(void)
 *
 * 割り込みをグローバルに無効化して古い状態を返す.
 * @return 無効にする前の割り込みの状態
 */
disable:
    .func disable
    mrs r0, cpsr
    cpsid i
    mov pc, lr
    .endfunc

/**
 * @fn irqmask restore(irqmask)
 *
 * グローバル割り込みマスクを以前の状態に復元する.
 * @param im
 *     復元する割り込み状態の割り込みマスク
 * @return 呼び出された際の割り込みの状態
 */
restore:
    .func restore
    msr cpsr_c, r0
    mov pc, lr
    .endfunc
