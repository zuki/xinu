/**
 * @file start.S
 *
 * Initialization code for Embedded Xinu on the Raspberry Pi.
 *
 * Embedded Xinu, Copyright (C) 2013.  All rights reserved.
 *
 * -----------------------------------------------------------------------------------------
 *
 * Specific for Raspberry Pi 3 (BCM2837) - Memory Layout (Not to scale)
 * 1GB OF RAM (PHYSICAL MEMORY) AVAILABLE. 1GB == 0x3FFFFFFF
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

#include <arm.h>     /* For ARM_MODE_SYS */

#define NULLSTK 8192

.section .init
    .globl _start

    /* _start:  Xinuカーネルのエントリポイント。これはカーネルイメージの
     * 先頭に置かれるが、Raspberry PiではGPUによりアドレス 0x8000 に
     * ロードされる。 */

    .func _start
_start:
    /* atags (ARM boot tags)へのポインタを保存する。Pi 3はatagsを*渡して
     * おり*、アドレス0x100に渡している。Pi 1と同じように、そこにアクセス
     * する。Aarch32(ARMv8-A)の標準プロトコルではatagsポインタをレジスタr2に
     * 渡することになっている。なのでr3に格納し、atagsを解析するCコードから
     * アクセスできるようにする（実際はもうATAGは使用されていない）。 */
    ldr     r3, =atags_ptr
    cmp     r2, #0
    moveq   r2, #0x100
    str     r2, [r3]

    /* プロセッサがすでにSYSTEMモードになっていないかチェックする。
     * もしそうなら、リセットハンドラへジャンブする。 */
    mrs    r0, cpsr
    and    r0, r0, #0b11111
    cmp    r0, #ARM_MODE_SYS
    beq    reset_handler


    /* ブート時にデバイスはHYP（ハイパーバイザー）モードにロードされるが、
     * このモードはCPSオペコードにアクセスする権限を持っていない。そのため、
     * 最も権限の高いモードであるSYSTEMモード（11111）に入るには、まず
     * SUPERVISORモードに入る必要がある。これによりより多くの権限が与えられ
     * "cps" オペコードを使用してSYSTEMモードに移行することができるようになる。
     * まず、CPSRのビットをクリアしてからスーパーバイザーモードに移行する。
     * プログラムカウンタを4つインクリメントして、32ビットワードをスキップする。
     * 例外から返ってくるとCPSRは変更され、SUPERVISORモードになっている。
     * これはリセットハンドラに続き、そこでSYSTEMモードに設定される。 */

    mrs    r0, cpsr
    orr    r0, r0, #ARM_MODE_SYS
    msr    spsr_cxsf, r0
    add    r0, pc, #4
    msr    ELR_hyp, r0
    eret

    /* リセットハンドラに続く */
    b reset_handler
    .endfunc

/* ARM 例外ベクタテーブル。これはVBARレジスタにコピーされる。 */
/* ベクタテーブルは32バイトアラインでなければならない */
.balign    0x20
.globl _vectors
_vectors:
    ldr pc, reset_addr      /* リセットハンドラ */
    ldr pc, undef_addr      /* 未定義命令例外ハンドラ */
    ldr pc, swi_addr        /* ソフトウェア割り込み例外ハンドラ */
    ldr pc, prefetch_addr   /* プリフェッチアボート例外ハンドラ */
    ldr pc, abort_addr      /* データアボート例外ハンドラ */
    ldr pc, reserved_addr   /* 予約済 */
    ldr pc, irq_addr        /* IRQ (Interrupt request) 例外ハンドラ */
    ldr pc, fiq_addr        /* FIQ (Fast interrupt request) 例外ハンドラ */

reset_addr:     .word reset_handler
undef_addr:     .word reset_handler
swi_addr:       .word reset_handler
prefetch_addr:  .word reset_handler
abort_addr:     .word reset_handler
reserved_addr:  .word reset_handler
irq_addr:       .word irq_handler
fiq_addr:       .word reset_handler

_endvectors:

.section .text

    /* reset_handler: カーネル起動時、ARMプロセッサのリセット時、（現在の
     * ところは）未処理の例外発生時に、実行されるリセットハンドルルーチン
     */
    .func reset_handler
reset_handler:

    /* SUPERVISORモードに入った後、ARMプロセッサが既知の状態になるように、
     * すべての割り込みを禁止してSYSTEMモードに入る。 */
    cpsid if
    /* ARM例外テーブルを.initセクションからアドレス0にコピーする。
     * 絶対アドレステーブルも同時にコピーする。ここでは全部で
     * 16ワードであると仮定している */
    mov   r0, #0
    ldr   r1, =_vectors
    ldmia r1!, {r2-r9}
    stmia r0!, {r2-r9}
    ldmia r1!, {r2-r9}
    stmia r0!, {r2-r9}

    mov r1, #0
    mcr p15, 0, r1, c12, c0, 0    /* VBARに例外テーブルのアドレスを書き込む */

    /* ARMプロセッサの制御レジスタを変更して次の機能を有効にする。
     *
     * - unaligned memory accesses   (bit 22)
     *
     * 注意: Xinuは意図的に非整列メモリアクセスを行うことはないはずだが、
     * gccが文字列のコピーなどで非整列メモリアクセスを生成することがある
     * ことがわかった（代替案としては、そうしないようにgccに
     * -mno-unaligned-accessを渡すことだろう）。
     */

    mrc p15, 0, r0, c1, c0, 0
    orr r0, #1 << 22
    mcr p15, 0, r0, c1, c0, 0

    /* カーネルの.bssセクションをクリアする.  */
    ldr r0, =_bss
    ldr r1, =_end
    mov r2, #0
    mov r3, #0
    mov r4, #0
    mov r5, #0
    b bssloopa

bssloopb:
    stmia r0!, {r2-r5}

bssloopa:
    cmp r0, r1      /* まだbssの終わりに届いていないかチェックする */
    blo bssloopb    /* if (r0 < r1) goto bssloopb; */

/* 以下はマルチコアの各NULLタスク用のスタックの初期化 */
    ldr r3, =core_init_sp

    // Put the null thread's stack directly after the kernel image.
    add sp, r1, #NULLSTK    // Core 0 stack pointer
    str sp, [r3, #0]

    add r0, sp, #NULLSTK    // Core 1 stack pointer
    str r0, [r3, #4]

    add r1, r0, #NULLSTK    // Core e stack pointer
    str r1, [r3, #8]

    add r2, r1, #NULLSTK    // Core 3 stack pointer
    str r2, [r3, #12]

    /* ARMが使用できる残りのメモリはXinuの"memheap"領域となり
     * 動的メモリ割り当てに使用される。その開始アドレスを設定する。  */
    ldr r0, =memheap
    str r2, [r0]        // コア3に割り当てた最後のスタックアドレスのr2を使う

    /* FPUを有効にする */
    mov r0, #0xf00000           // cp10とc11をすべてアクセス可に - pi3 - 4.5.29参照
    mcr p15, 0, r0, c1, c0, 2   // CPACRにr0を書き込む
    isb
    mov r1, #0x40000000         // FPUを有効にするためFPEXC_ENビットをセットする
    vmsr FPEXC, r1
    isb

    /* Raspberry Piのグラフィックサブシステムを初期化する */
    bl screenInit

    /* xinu/system/initialize.cにあるnulluser関数に分岐する。
    /* この関数はシステムを初期化してnullスレッドとなる */
    b    nulluser
    .endfunc
