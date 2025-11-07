/**
 * @file setupStack.c
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */
#include <kernel.h>
#include <platform.h>
#include <arm_64.h>

/**
 * @ingroup bcm2837
 * ワード単位のARM64コンテキストレコード数（x19-x30, daif, procaddr)
 */
#define CONTEXT_WORDS 14

/**
 * @ingroup bcm2837
 * 標準的なARM64呼び出し規約では最初の8引数はx0-x7で渡し、残りはスタックで渡す
 */
#define MAX_REG_ARGS 8

/**
 * @ingroup bcm2837
 *
 * 新規スレッドのスタックにコンテキストレコードと引数をセットする
 * (ARM版).
 * @param stackaddr スタックアドレス
 * @param procaddr  プロセスアドレス
 * @param retaddr   リターンアドレス
 * @param nargs     引数の数
 * @param ap        引数リスト
 * @return スタックのトップのアドレス
 */
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

    /* 新規スレッドためのコンテキストレコードを構築する */
    saddr -= CONTEXT_WORDS;
    saddr[0] = 0;

    /* レジスタで渡される引数（コンテキストレコードの一部） */
    for (i = 0; i < reg_nargs; i++)
    {
        saddr[i] = va_arg(ap, unsigned long);
    }

    /* DAIFレジスタ : bit[9-6]=DAIF, Iのみ有効とする */
    saddr[i++] = 0x0340UL;

    for (; i < CONTEXT_WORDS - 2; i++)
    {
        saddr[i] = 0;
    } 

    /* リターンアドレス  */
    saddr[CONTEXT_WORDS - 2] = (unsigned long)retaddr;

    /* プログラムカウンタ  */
    saddr[CONTEXT_WORDS - 1] = (unsigned long)procaddr;

    /* スタック渡しの引数（コンテキストレコードではない）  */
    for (i = 0; i < spilled_nargs; i++)
    {
        saddr[CONTEXT_WORDS + i] = va_arg(ap, unsigned long);
    }
#if 0
    kprintf("&stack: 0x%x, stack: 0x%x, proc: 0x%x, ret: 0x%x, nargs: %d\n", 
        &saddr, stackaddr, procaddr, retaddr, nargs);
    for (int j=0; j < CONTEXT_WORDS; j++) {
        kprintf("saddr[%d] = 0x%x\n", j, saddr[j]);
    }
    kprintf("\n");
#endif
    /* スタックの「トップ」（最下位のアドレス）を返す  */
    return saddr;
}
