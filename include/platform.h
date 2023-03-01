/**
 * @file platform.h
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

/** プラットフォーム名とファミリ名の終端文字を含む最大文字列長.  */
#define PLT_STRMAX 18

/**
 * ブート時にplatforminit()関数で設定される種々のプラットフォーム固有のパラメタ.
 * platforminit()が呼び出された際は、すべて0にセットされている。
 */
struct platform
{
    /**
     * NULL終端されたプラットフォームの名前.
     * 何を"名前"または"ファミリー"とみなすかはプラットフォーム次第である。
     */
    char name[PLT_STRMAX];

    /**
     * NULL終端されたプラットフォームのファミリー.
     * 何を"名前"または"ファミリー"とみなすかはプラットフォーム次第である。
     */
    char family[PLT_STRMAX];

    /**
     * CPUが利用可能な最小の物理アドレス.  利用可能な物理メモリが0から
     * 始まる場合、platforminit()はこの値をそのままにしておくことができる。
     */
    void *minaddr;

    /**
     * CPUが利用可能な最小の物理アドレス+1.
     * platforminit()で設定する必要がある。
     */
    void *maxaddr;

    /**
     * ARM Cortex A53はL1データキャッシュを最大64KBまで使用できる.
     * Raspbery Pi 3 Model B+ はL1キャッシュが有効になっている。
     * この値はplatforminit()でCortex A53 CCSIDRを参照して取得する。
     */
    int dcache_size;

    /**
     * システムタイマーの周波数（1秒あたりのサイクル数）.
     * RTCLOCKを有効にした場合は、platforminit()で設定する必要がある。
     * clkcount()が返す値が変化する頻度である。
     */
    unsigned long clkfreq;

    /**
     * UART除数ラッチLS. UARTドライバが必要とする場合だけ
     * platforminit() は設定する必要がある。
     */
    uint8_t uart_dll;

    /**
     * UART IRQラインの番号.  UARTドライバが必要とする場合だけ 　* * * platforminit() は設定する必要がある。
     */
    uint8_t uart_irqnum;

    /**
     * ボードのシリアル番号の下位ワード. 設定はオプション。
     * 現在のところ、SMSC LAN9512ドライバでしか仕様されていない。
     */
    unsigned int serial_low;

    /**
     * ボードのシリアル番号の上位ワード. 設定はオプション。
     * 現在のところ、SMSC LAN9512ドライバでしか仕様されていない。
     */
    unsigned int serial_high;
};

extern struct platform platform;    // initialize.cで定義

/* Max RAM addresses */
#define MAXADDR_DEFAULT  0x00800000 /**< default  8MB RAM */
#define MAXADDR_WRT54G   0x00800000 /**< G    has 8MB RAM */
#define MAXADDR_WRT54GL  0x01000000 /**< GL   has 16MB RAM */
#define MAXADDR_WRT350N  0x02000000 /**< 350N has 32MB RAM */
#define MAXADDR_WRT160NL 0x02000000 /**< 160NL has 32MB RAM */

/* Time Base Frequency */
#define CLKFREQ_DEFAULT  100000000
#define CLKFREQ_WRT54G   120000000
#define CLKFREQ_WRT54GL  100000000
#define CLKFREQ_WRT350N  150000000
#define CLKFREQ_WRT160NL 200000000

/* UART DLL (Divisor Latch LS) */
#define UART_DLL_DEFAULT    0x0B
#define UART_DLL_WRT54G     0x0E
#define UART_DLL_WRT54GL    0x0B
#define UART_DLL_WRT350N    0x29

/* Used internally by create()  */
void *setupStack(void *stackaddr, void *procaddr,
                 void *retaddr, unsigned int nargs, va_list ap);

int platforminit(void);

#endif                          /* _PLATFORM_H_ */
