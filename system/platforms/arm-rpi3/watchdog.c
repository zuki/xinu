/**
 * @file watchdog.c
 *
 * このファイルにはRaspberry Piで使用されているBCM2835 SoCの
 * Watchdog Timerを設定するコードが含まれている。ウォッチドッグ
 * タイマーを有効にすると、ウォッチドッグタイマーが切れる前に
 * リフレッシュする必要があり、さもなければシステムがリセットされる。
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <stddef.h>
#include "bcm2837.h"

#define PM_RSTC                       (PM_REGS_BASE + 0x1c)
#define PM_WDOG                       (PM_REGS_BASE + 0x24)

#define PM_PASSWORD                   0x5a000000
#define PM_RSTC_WRCFG_CLR             0xffffffcf
#define PM_RSTC_WRCFG_FULL_RESET      0x00000020

#define PM_WDOG_UNITS_PER_SECOND      (1 << 16)
#define PM_WDOG_UNITS_PER_MILLISECOND (PM_WDOG_UNITS_PER_SECOND / 1000)

/**
 * ウォッチドッグタイマーをセットする.
 *
 * @param msecs
 *      システムがリセットするまで待機するミリ秒数.
 *
 * @return
 *      OK
 */
syscall watchdogset(uint msecs)
{
    *(volatile uint*)PM_WDOG = PM_PASSWORD | (msecs * PM_WDOG_UNITS_PER_MILLISECOND);
    uint cur = *(volatile uint*)PM_RSTC;
    *(volatile uint*)PM_RSTC = PM_PASSWORD | (cur & PM_RSTC_WRCFG_CLR) |
                               PM_RSTC_WRCFG_FULL_RESET;
    return OK;
}
