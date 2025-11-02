/**
 * @file ctxsw.S
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

.text
    .align 4
    .globl ctxsw
    .type ctxsw, "function"
/**
 *  @ingroup threads
 *-----------------------------------------------------------------------
 *  ctxsw  -  スレッドコンテキストを切り替える.
 *------------------------------------------------------------------------
 * 詳細はplatforms/arm-rpi3/ctxsw.Sのコメントを参照
 *------------------------------------------------------------------------*/
/**
 * @fn void ctxsw(&oldregs, &newregs)
 *
 * 現在のプロセッサの情報を保存して別のプロセスにコンテキスト
 * （レジスタ値）を切り替える. 
 *
 * @param  oldregs 現プロセスのレジスタ保存領域のアドレス
 * @param  newregs 新プロセスのレジスタ保存領域のアドレス
 * @return 特例。上記参照。
 */
ctxsw:
    dsb     sy; isb
    /* Save old callee-saved registers. */
    /* str     q0, [sp, #-16]! */
    stp     x19, x9, [sp, #-16]!
    stp     x21, x20, [sp, #-16]!
    stp     x23, x22, [sp, #-16]!
    stp     x25, x24, [sp, #-16]!
    stp     x27, x26, [sp, #-16]!
    stp     x29, x28, [sp, #-16]!
    stp     x30, x30, [sp, #-16]!

    /* Switch stacks. */
    mov     x9, sp
    str     x9, [x0]
    mov     sp, x1

    /* Load new callee-saved registers. */
    ldp     x15, x30, [sp], #16
    ldp     x29, x28, [sp], #16
    ldp     x27, x26, [sp], #16
    ldp     x25, x24, [sp], #16
    ldp     x23, x22, [sp], #16
    ldp     x21, x20, [sp], #16
    ldp     x19, x9, [sp], #16
    /* ldr     q0, [sp], #16 */

    /* Why not simply ret by x30? This is a hack for forkret. */
    dsb     sy; isb
    ret     x15
