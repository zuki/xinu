/**
 * @file irq_handler.S
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <arm.h>  /* Needed for ARM_MODE_SYS definition.  */

.globl irq_handler

/**
 * Xinuの割り込みハンドラのエントリーポイント（ARM版）。注意: 実際には
 * ARMが提供するIRQモードは使用せず、代わりにプロセッサをSYSモードに
 * 切り替えてSYSモードのスタックを使用する。これによりARMが他のCPUとより
 * 近いものになり、コンテキストスイッチのコードで固有な処理を削除する
 * ことができる。しかし、この方法は、IRQハンドラは現在実行中のスレッド
 * （割り込まれたスレッド）のスタックを使用することに注意が必要である。
 * このため、割り込みを有効にして実行するスレッドのスタックサイズは
 * 少なくともスレッド自身が必要とするスタックスペースに、任意の
 * IRQハンドラが必要とする最大スタックスペースを加えたサイズに設定する
 * 必要がある。
 */
irq_handler:
    .func irq_handler

    /* LR_irqを修正する。これは、ARMプロセッサがIRQハンドラを呼び出す
     * 場合の癖である */
    sub lr, lr, #4

    /* srsdb (Store Return State Decrement Before) 命令 */
    /* SYSモードのスタックにリターンステートを格納する。これには
     * 割り込まれる前のSYSモードのCPSRであるSPSR_irqと、割り込まれた
     * スレッドの実行を継続するために戻らなければならないアドレスで
     * あるLR_irqが含まれる。 */
    srsdb #ARM_MODE_SYS!

    /* [Change Program State Interrupt Disable] */
    /* IRQとFIQは無効のままでSYSモードに移行する */
    cpsid if, #ARM_MODE_SYS

    /* SYSモードスタックに上書きされる可能性のあるレジスタ、すなわち、
     * SYSモードLRと他のすべてのcaller-save汎用レジスタを保存する。
     * また、スタックポインタを8バイト境界に揃えるためにデクリメント
     * した量を格納できるようにするためにr4を保存する（以下のコメント参照）。
     */
    push {r0-r4, r12, lr}

    /* 「ARMアーキテクチャのプロシージャコール規則」によるとスタック
     * ポインタは常時4バイトアライメントであるが、外部から見える関数を
     * 呼び出す際は8バイトアライメントでなければならない。これは重要である。
     * なぜなら、このコードはIRQから到達するため、現在スタックは4バイト
     * 境界にしかアライメントされていない可能性があるためである。そうで
     * ある場合、dispatch()を呼び出す前にスタックを8バイト境界にパディング
     * する必要がある。  */
    and r4, sp, #4
    sub sp, sp, r4

    /* BCM2835ドキュメントにしたがい、データメモリバリアを実行する  */
    bl dmb

    /* Cのあり込みdispatchコードを呼び出す */
    bl dispatch

    /* BCM2835ドキュメントにしたがい、データメモリバリアを実行する  */
    bl dmb

    /* 元のスタックアライメントを復元する（8バイトアライメントに関する
     * 注記を参照）  */
    add sp, sp, r4

    /* SYSモードスタックから上で述べたレジスタを復元する */
    pop {r0-r4, r12, lr}

    /* [Return From Exception Increment After] */
    /* SYSモードスタックに格納あsれていた元のSYSモードCPSRとPCをロードする  */
    rfeia sp!

    .endfunc
