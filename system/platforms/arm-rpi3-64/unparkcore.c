/**
 * @file unparkcore.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009. All rights reserved */

#include <stddef.h>
#include <stdint.h>
#include <thread.h>
#include <core.h>
#include <mmu_64.h>
#include <clock.h>

extern void CoreSetup(void);
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
uintptr_t core_init_sp[4];

/**
 * @ingroup bcm2837
 * 各コアに渡される引数へのポインタを保管する配列
 */
void *init_args[4];

/**
 * @ingroup bcm2837
 *
 * プロセッサコアにイベントを送信し、起動時の待機状態から「解除
 * （アンパーキング）」する. これは関数アドレスをメールボックスに
 * ロードすることで行う。注意: この操作は初期化時に一度だけ行う
 * 必要があり、その後コンテキストスイッチを行うことができるように
 * なる。使用法はinitialize.c を参照。
 * (訳注: armstub7ではunpark後に実行される関数のアドレスは
 * Core N Mailbox3 Setレジスタに書き込むことになっている。
 * この点、spin_cpu0+0x8*numのメモリアドレスに書き込むことに
 * なっているarmstub8と異なる点に注意。
 * ここに書き込んだデータはCore N Mailbox 3 Rd/Clrから読み込む
 * ことができる。ここではsev()で気象されたコアはCoreSetup()を
 * 実行することになる)
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
        /* 0xd8: spin_cpu0, 0x8: addr指定用の8バイト */
        *(volatile fn *)(0xd8UL + 0x8UL * num) = CoreSetup;
    }
}

/**
 * @ingroup bcm2837
 *
 * ヌルスレッドを作成する（テスト用なので削除すべき）.
 */
void createnullthread(void)
{
    uint32_t cpuid;
    cpuid = getcpuid();

    /* enable interrupts */
    //XXX    enable();

    while(TRUE)
    {
        kprintf("CORE %d IS RUNNING\r\n", cpuid);
        udelay(250);
    }
}
