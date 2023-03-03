/**
 * @file unparkcore.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009. All rights reserved */

#include <stddef.h>
#include <thread.h>
#include <core.h>
#include <mmu.h>
#include <clock.h>

extern void CoreSetup(void) __attribute__((naked));
typedef void (*fn)(void);
extern void sev(void);

/**
 * @ingroup bcm2837
 * 各コアの開始地点のアドレスを保管する配列
 */
void *corestart[4];

/**
 * @ingroup bcm2837
 * 各コアの初期スタックポインタを保管する配列.
 * 値はstart.Sでセットされる
 */
unsigned int core_init_sp[4];

/**
 * @ingroup bcm2837
 * 各コアに渡される引数へのポインタを保管する配列
 */
void *init_args[4];

/**
 * @ingroup bcm2837
 *
 * プロセッサコアにイベントを送信し、起動時の待機状態から「解除
 * （アンパーキング）」する. これは関数をメールボックスにロードする
 * ことで行う。注意: この操作は初期化時に一度だけ行う必要があり、
 * その後コンテキストスイッチを行うことができるようになる。使用法は
 * initialize.c を参照。
 *
 * @param num       待機解除するコアの番号
 * @param procaddr  開始するスレッドのアドレス
 * @param args      渡す引数
 */
void unparkcore(int num, void *procaddr, void *args) {
    udelay(5);
    if (num > 0 && num < 4)
    {
        corestart[num] = (void *) procaddr;
        init_args[num] = args;
        sev();                              // イベントを送信
        *(volatile fn *)(CORE_MBOX_BASE + CORE_MBOX_OFFSET * num) = CoreSetup;
    }
}

/**
 * @ingroup bcm2837
 *
 * ヌルスレッドを作成する（テスト用なので削除すべき）.
 */
void createnullthread(void)
{
    uint cpuid;
    cpuid = getcpuid();

    /* enable interrupts */
    //XXX    enable();

    while(TRUE)
    {
        kprintf("CORE %d IS RUNNING\r\n", cpuid);
        udelay(250);
    }
}
