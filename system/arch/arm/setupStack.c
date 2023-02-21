/**
 * @file setupStack.c
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <stdint.h>
#include <platform.h>
#include <arm.h>

/** ワード単位のARMコンテキストレコード数（r0-r11, cpsr, lr, pcを含む) */
#define CONTEXT_WORDS 15

/** 標準的なARM呼び出し規約では最初の4引数はr0-r3で渡し、
 * 残りはスタックで渡す  */
#define MAX_REG_ARGS 4

/** 新規スレッドのスタックにコンテキストレコードと引数をセットする
 * (ARM版)  */
void *setupStack(void *stackaddr, void *procaddr,
                 void *retaddr, unsigned int nargs, va_list ap)
{
    unsigned int spilled_nargs;     // スタック渡しの引数の数
    unsigned int reg_nargs;         // レジスタ渡しの引数の数
    unsigned int i;
    uintptr_t *saddr = stackaddr;

    /* （コンテキストレコード以外に）スタックで渡す引数があるか判断する
     * もしあれば、そのためのスペースを予約する  */
    if (nargs > MAX_REG_ARGS) {
        spilled_nargs = nargs - MAX_REG_ARGS;
        reg_nargs = MAX_REG_ARGS;
        saddr -= spilled_nargs;     // 引数用のスペースを予約
    } else {
        spilled_nargs = 0;
        reg_nargs = nargs;
    }

    /* 新しいスレッドがコンテキストレコードをポップオフした後、
     * スタックが8バイト境界にアラインされるように1ワード
     * スキップする可能性がある  */
    if ((unsigned long)saddr & 0x4)
    {
        --saddr;
    }

    /* 新規スレッドためのコンテキストレコードを構築する */

    saddr -= CONTEXT_WORDS;

    /* レジスタで渡される引数（コンテキストレコードの一部） */
    for (i = 0; i < reg_nargs; i++)
    {
        saddr[i] = va_arg(ap, uintptr_t);
    }

    for (; i < CONTEXT_WORDS - 3; i++)
    {
        saddr[i] = 0;
    }

    /* プログラムステータスレジスタのコントロールビット
     * (SYSモード, IRQははじめから有効 */
    saddr[CONTEXT_WORDS - 3] = ARM_MODE_SYS | ARM_F_BIT;

    /* リターンアドレス  */
    saddr[CONTEXT_WORDS - 2] = (uintptr_t)retaddr;

    /* プログラムカウンタ  */
    saddr[CONTEXT_WORDS - 1] = (uintptr_t)procaddr;

    /* スタック渡しの引数（コンテキストレコードではない）  */
    for (i = 0; i < spilled_nargs; i++)
    {
        saddr[CONTEXT_WORDS + i] = va_arg(ap, uintptr_t);
    }

    /* スタックの「トップ」（最下位のアドレス）を返す  */
    return saddr;
}
