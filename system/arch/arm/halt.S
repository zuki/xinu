/**
 * @file halt.S
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

.globl halt

#include <arm.h>

/**
 * @fn void halt(void)
 *
 * プロセッサを無限ループに送り込むことによりシステムを停止する.
 */
halt:
    .func halt

_loop:
    /* IRQとFIQを無効にする  */
    msr cpsr_c, #ARM_I_BIT | ARM_F_BIT

    /* 割り込みを待機  */
    wfi

    /* 必要ないはずだが、念の為  */
    b _loop
    .endfunc
