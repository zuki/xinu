/**
 * @file timer.c
 *
 * このファイルにはRaspberry Piで使用されているBCM2835 SoCの
 * システムタイマーとインターフェイスするコードが含まれている。
 *
 * このハードウェアの詳細については、BCM2835-ARM-Peripherals.pdf の
 * "12. システムタイマー"を参照されたい。しかし、このドキュメントには、
 * タイマーの動作周波数（1MHzである）、出力コンペアレジスタの一部
 * （0と2が該当）がGPUによって使用されるためARMコードで使用できないこと、
 * システムタイマーがどの割り込みラインに接続されているか（IRQs 0-3が
 * 順に出力コンペアレジスタ0-3にマッピングされている）が記載されて
 * いない。
 *
 * このハードウェアの詳細については、http://xinu-os.org/BCM2835_System_Timer
 * を参照されたい。
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <clock.h>
#include "bcm2837.h"

/** BCM2835 システムタイマーレジスタのレイアウト.  */
struct bcm2835_timer_regs {
    uint CS;  /** システムタイマー コントロール/ステータス */
    uint CLO; /** システムタイマー カウンター下位32ビット */
    uint CHI; /** システムタイマー カウンター上位32ビット */
    uint C0;  /** システムタイマー コンペア 0.  使用不可; GPUで使用済み  */
    uint C1;  /** システムタイマー コンペア 1 */
    uint C2;  /** システムタイマー コンペア 2.  使用不可; GPUで使用済み  */
    uint C3;  /** システムタイマー コンペア 3 */
};

#define BCM2835_SYSTEM_TIMER_MATCH_0 (1 << 0)
#define BCM2835_SYSTEM_TIMER_MATCH_1 (1 << 1)
#define BCM2835_SYSTEM_TIMER_MATCH_2 (1 << 2)
#define BCM2835_SYSTEM_TIMER_MATCH_3 (1 << 3)

static volatile struct bcm2835_timer_regs * const regs =
        (volatile struct bcm2835_timer_regs*)SYSTEM_TIMER_REGS_BASE;

/* clkcount()のインタフェースはclock.hに記載されている  */
/**
 * @brief システムタイマーカウンタを返す.
 *
 * @details Raspberry-Pi固有の注記: この関数はBCM2835のフリーランニング
 * カウンタの下位32ビットを返す。このカウンタは1MHzで稼働するので、
 * 4295秒ごとにオーバーフローする。
 *
 * @return BCM2835のフリーランニングカウンタの下位32ビット
 */
unsigned long clkcount(void)
{
    unsigned long count;

    pre_peripheral_read_mb();

    count = regs->CLO;

    post_peripheral_read_mb();

    return count;
}

/* clkupdate() のインタフェースはclock.hに記載されている  */
/**
 * システムタイマーカウンタを指定のサイクル数で更新する。
 *
 * @param cycles 次に割り込みを発生させるサイクル数
*/
void clkupdate(unsigned long cycles)
{
    pre_peripheral_write_mb();

    /* タイマーを設定するために、システムタイマーの出力コンペア
     * レジスタのうち、GPUが使用していないC3（System Timer Compare 3）を
     * 使用する。タイマーの下位32ビット(CLO)がC3と一致すると、ハード
     * ウェアはタイマー割り込みを発生させる。
     *
     * まず、タイマーコントロール/ステータスレジスタの
     * BCM2835_SYSTEM_TIMER_MATCH_3 ビットをクリアして、現在の割り込みを
     * クリアする。Broadcom のドキュメントによると、これにはクリアする
     * ビットに 1 を書き込めばよい。 */
    regs->CS = BCM2835_SYSTEM_TIMER_MATCH_3;

    /* 次に、C3 出力コンペアレジスタに新しい値を設定する。これを行うには
     * カウンタの現在の下位ビットを読み出し、指定されたサイクル数を追加する。
     * 出力コンペアレジスタは設計上32ビットしかないため、ラップアラウンドは
     * 必ず正しく処理されることに注意されたい。  */
    regs->C3 = regs->CLO + cycles;

    post_peripheral_write_mb();
}
