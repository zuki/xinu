/**
 * @file ctxsw.S
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

.text
    .align 4
    .globl ctxsw
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
    mrs     x9, daif
    stp     x30, x30, [sp, #-16]!
    stp     x28, x29, [sp, #-16]!
    stp     x26, x27, [sp, #-16]!
    stp     x24, x25, [sp, #-16]!
    stp     x22, x23, [sp, #-16]!
    stp     x20, x21, [sp, #-16]!
    stp     x9,  x19, [sp, #-16]!

    /* Switch stacks. */
    mov     x9, sp
    str     x9, [x0]
    ldr     x10, [x1]
    mov     sp, x10

    /* Load new callee-saved registers. */
    ldp     x9,  x19, [sp], #16
    ldp     x20, x21, [sp], #16
    ldp     x22, x23, [sp], #16
    ldp     x24, x25, [sp], #16
    ldp     x26, x27, [sp], #16
    ldp     x28, x29, [sp], #16
    ldp     x30, x15, [sp], #16
    msr     daif, x9
    /* ldr     q0, [sp], #16 */

    /* Why not simply ret by x30? This is a hack for forkret. */
    dsb     sy; isb
    ret     x15
