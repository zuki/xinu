/**
 * @file start.S
 *
 * Initialization code for Embedded Xinu on the Raspberry Pi.
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */


#include <arm.h>     /* For ARM_MODE_SYS */

#define NULLSTK 8192

.section .init
	.globl _start

	/* _start:  Xinuカーネルのエントリポイント。これはカーネルイメージの
     * 先頭コードであり、Raspberry PiではGPUによりアドレス 0x8000 に
     * ロードされる */
	.func _start
_start:
	/* atgs (ARM boot tags) へのポインタを保存する。ブートローダは
     * これをR2経由で渡すはずだが、Linuxのデフォルト動作を前提とした
     * 古いブートローダに対処するため、この値が0だったら0x100で
     * 上書きする。（これは、たとえばBCM2835のブートローダが
     * 該当する）  */
	ldr r3, =atags_ptr  /* atags_ptrはsystem/platforms/arm-rpi/platforminit.cで定義 */
	cmp r2, #0          /* R2 = 0x100 if R2 == 0 */
	moveq r2, #0x100
	str r2, [r3]        /* *R3 = R2 */

	/* 続きはreset_handlerで行う  */
	b reset_handler
	.endfunc

/* ARM例外ベクタテーブル。これはアドレス0にコピーされる。
 * ARM Architecture Reference ManualのA2.6 "Exceptions" を参照 */
_vectors:
	ldr pc, reset_addr    /* リセットハンドラ */
	ldr pc, undef_addr	  /* 未定義命令例外ハンドラ */
	ldr pc, swi_addr	  /* ソフトウェア割り込み例外ハンドラ */
	ldr pc, prefetch_addr /* プリフェッチアボート例外ハンドラ */
	ldr pc, abort_addr	  /* データアボート例外ハンドラ */
	ldr pc, reserved_addr /* 予約済 */
	ldr pc, irq_addr	  /* IRQ (Interrupt request) 例外ハンドラ */
	ldr pc, fiq_addr	  /* FIQ (Fast interrupt request) 例外ハンドラ */

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

	/* reset_handler: リセットハンドラ関数はARMプロセッサがリセットされた、
     * または（現在）ハンドラのない例外が発生した際に実行され、カーネルを
     * 起動する。 */
	.func reset_handler
reset_handler:

	/* ARMプロセッサが既知の状態になるようにすべての割り込みを無効に
     * してSYSモードに入る。
     * （実際は、IRQとFIQを無効してSYSモードに移行） */
	cpsid if, #ARM_MODE_SYS

	/* ARM例外テーブル（絶対アドレステーブルを含む）を.init
     * セクションからアドレス0にコピーする。これでは全部で
     * 16ワードであると仮定している  */
	mov r0, #0
	ldr r1, =_vectors
	ldmia r1!, {r2-r9}
	stmia r0!, {r2-r9}
	ldmia r1!, {r2-r9}
	stmia r0!, {r2-r9}

	/* ARMプロセッサのコントロールレジスタを変更することにより
     * 次の機能を有効にする。
	 *
	 * - 非整列メモリアクエス   (bit 22)
	 *
	 * 注意: Xinuは意図的に非整列メモリアクセスを行うことはないはずだが、
     * gccが文字列のコピーなどで非整列メモリアクセスを生成することがある
     * ことがわかった（代替案としてはそうしないように、gccに
     * -mno-unaligned-accessを渡すことです）。 */
	mrc p15, 0, r0, c1, c0, 0
	orr r0, #1 << 22
	mcr p15, 0, r0, c1, c0, 0

	/* .bssセクションを0クリアする  */
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
	cmp r0, r1 	/* check that we still haven't hit the end of bss yet */
	blo bssloopb	/* if still below, go backwards and loop */

	/* カーネルイメージの直後にNULLスレッド用のスタックを置く  */
	add sp, r1, #NULLSTK

    /* ARMが使用できる残りのメモリはXinuの"memheap"領域となり
     * 動的メモリ割り当てに使用される。その開始アドレスを設定する。 */

	ldr r0, =memheap
	str sp, [r0]

	/* Raspberry Piのグラフィックサブシステムを初期化する  */
	bl screenInit

    /* プラットフォーム固有のCのstartup関数に分岐する。この関数は引数を取らず、復帰しない  */
	b nulluser
	.endfunc
