/**
 * @file start.S
 *
 * Embedded XinuのRaspberry Piポート用の初期化コード
 *
 * Embedded Xinu, Copyright (C) 2013.  All rights reserved.
 *
 * -----------------------------------------------------------------------------------------
 *
 * Raspberry Pi 3 (BCM2837)固有のメモリレイアウト（スケールは正しくない）
 * 1GBのRAM (物理メモリ) が利用できる. 1GB == 0x3FFFFFFF
 * +----------+
 * |          |
 * | IO SPACE | -> 0x3F000000 - 0x3FFFFFFF IO用に予約 (GPIO, UART, SYS TIMER, USB CORE)
 * |          |
 * +----------+
 * |          |
 * |          |
 * |   HEAP   | -> getmem はここから割り当てる
 * |          |
 * |          |
 * +----------+
 * | OS STACK | -> null プロセスのスタックになる
 * +----------+
 * |   BSS    | -> C 環境で必要
 * +----------+
 * |   TEXT   | -> XINUのコード
 * |          |
 * +----------+
 * | RESERVED | -> 割り込みハンドラとベクタ
 * +----------+
 */

#include <sysregs_64.h>
#include <mmu_64.h>

#define NULLSTK 8192

.section .init

.globl _start
_start:
    /* 現在の例外レベルを取得して、対応する処理に分岐する */
    mrs     x9, CurrentEL
    and     x9, x9, #0xc
    cmp     x9, #8
    beq     el2
    bgt     el3

el3:
    mov     x9, #SCR_VALUE
    msr     scr_el3, x9
    mov     x9, #SPSR_EL3_VALUE
    msr     spsr_el3, x9
    adr     x9, el2
    msr     elr_el3, x9
    eret

el2:
    /*
     * HCR_EL2.RW, bit[31] = 1: The Execution state for EL1 is AArch64.
     * The Execution state for EL0 is determined by the current
     * value of PSTATE.nRW when executing at EL0.
     */
    mov     x9, #HCR_VALUE
    msr     hcr_el2, x9

    /* Setup SCTLR access. */
    ldr     x9, =SCTLR_VALUE_MMU_DISABLED
    msr     sctlr_el1, x9

    /* Enable SIMD instructions. */
    ldr     x9, =CPACR_VALUE
    msr     cpacr_el1, x9

    /* Change execution level to EL1. */
    mov     x9, #SPSR_EL2_VALUE
    msr     spsr_el2, x9
    adr     x9, el1
    msr     elr_el2, x9
    eret

el1:
    adr     x9, kpgdir
 
    /* Higher and lower half map to same physical memory region. */
    msr     ttbr0_el1, x9
    msr     ttbr1_el1, x9
 
    ldr     x9, =TCR_VALUE
    msr     tcr_el1, x9
 
    ldr     x9, =MAIR_VALUE
    msr     mair_el1, x9
 
    isb

    /* mask fiq, irq */
    msr daifclr, #3
    /* VBARに例外テーブルのアドレスを書き込む */
    ldr x0, =vectors
    msr vbar_el1, x0

    /* bssのクリア */
    ldr x1, =_bss
    ldr x2, =_end
1:
    cmp x1, x2
    beq 2f
    stp xzr, xzr, [x1], #16
    b 1b

2:
/* マルチコアの各NULLタスク用のスタックの初期化 */
/* NULLSTK = 8192 */
    ldr x3, =core_init_sp

    /* カーネルイメージの直後にnullスレッドのスタックを置く */
    add sp, x2, #NULLSTK    // Core 0 stack pointer
    mov x0, sp
    str x0, [x3, #0]

    add x1, x0, #NULLSTK    // Core 1 stack pointer
    str x1, [x3, #8]

    add x2, x1, #NULLSTK    // Core e stack pointer
    str x2, [x3, #16]

    add x4, x2, #NULLSTK    // Core 3 stack pointer
    str x4, [x3, #24]

/* ARMが使用できる残りのメモリはXinuの"memheap"領域となり
 * 動的メモリ割り当てに使用される。その開始アドレスを設定する。
 * memheapはsystem/initialize.cで定義 */
    ldr x0, =memheap
/* コア3に割り当てた最後のスタックアドレスのx2を使う */
    str x4, [x0]        

/* xinu/system/initialize.cにあるnulluser関数に分岐する。
/* この関数はシステムを初期化してnullスレッドとなる */
    b   nulluser
