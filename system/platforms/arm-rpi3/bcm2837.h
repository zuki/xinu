/**
 * @file bcm2837.h
 * @ingroup bcm2837
 *
 * Raspberry Piで使用されているBCM2837 SoC固有の定義.
 *
 * なお、このファイルで定義されている数値の一部はBroadcomのドキュメント
 * "BCM2835 ARM Peripherals"に記載されているが、残念ながらLinuxのソース
 * （arch/arm/mach-bcm2708/include/mach/platform.h）でしか確認できない
 * ものもある。
 */

#ifndef _ARM_BCM2837_H_
#define _ARM_BCM2837_H_

#include <stddef.h>

/********************************************************************
 * BCM2835ペリフェラルのARM物理メモリアドレス                       *
 ********************************************************************/
/** メモリマップドペリフェラルアドレス空間の開始アドレス  */
#define PERIPHERALS_BASE 0x3F000000

/** システムタイマー  */
#define SYSTEM_TIMER_REGS_BASE (PERIPHERALS_BASE + 0x3000)

/** 割り込みコントローラ（ARM用）  */
#define INTERRUPT_REGS_BASE    (PERIPHERALS_BASE + 0xB200)

/** メールボックス  */
#define MAILBOX_REGS_BASE      (PERIPHERALS_BASE + 0xB880)

/** 電源管理 / ウォッチドッグタイマー  */
#define PM_REGS_BASE           (PERIPHERALS_BASE + 0x100000)

/* GPIO */
#define GPIO_REGS_BASE         (PERIPHERALS_BASE + 0x200000)

/** PL011 UART  */
#define PL011_REGS_BASE        (PERIPHERALS_BASE + 0x201000)

/** SD ホストコントローラ  */
#define SDHCI_REGS_BASE        (PERIPHERALS_BASE + 0x300000)

/** Synopsys DesignWare Hi-Speed USB 2.0 On-The-Go (DWC) コントローラ  */
#define DWC_REGS_BASE          (PERIPHERALS_BASE + 0x980000)


/** ***********************************************************************
 * BCM2835ペリフェラルのIRQラインの一部. ここで使用されている番号に       *
 * 関する注記: IRQ 0-63 はGPUとCPUで共用されているが、IRQ 64+ はCPU専用で *
 * ある。                                                                 *
 **************************************************************************/

/** システムタイマー: 出力コンペアレジスタごとに1 IRQ  */
#define IRQ_SYSTEM_TIMER_0 0
#define IRQ_SYSTEM_TIMER_1 1
#define IRQ_SYSTEM_TIMER_2 2
#define IRQ_SYSTEM_TIMER_3 3

/** デフォルトで使用するタイマーIRQ.  注記: これは 1 か 3 のいずれかに限る。
 * 0 と 2 はVideoCoreで使用されているからである。  */
#define IRQ_TIMER          IRQ_SYSTEM_TIMER_3

/** USBホストコントローラ (DWC)  */
#define IRQ_USB            9

/** miniUART */
#define IRQ_AUX            29

/** PCMサウンド  */
#define IRQ_PCM            55

/** PL011 UART  */
#define IRQ_PL011          57

/** SDカードホストコントローラ  */
#define IRQ_SD             62


/** *********************************
 * ボード固有の電源管理             *
 ************************************/

enum board_power_feature {
    POWER_SD     = 0,
    POWER_UART_0 = 1,
    POWER_UART_1 = 2,
    POWER_USB    = 3,
};

extern int bcm2837_setpower(enum board_power_feature feature, bool on);
extern void bcm2837_power_init(void);
#define board_setpower bcm2837_setpower


/** *********************************************************************
 * ペリフェラルアクセうに使用するメモリバリアのインタフェース.          *
 *                                                                      *
 * これらは、Broadcomの"BCM2835 ARM Peripherals"ドキュメントの          *
 * セクション 1.3に記載されているメモリの順序に関する注意事項を         *
 * 満たすために必要である。                                             *
 ************************************************************************/

extern void dmb(void);

extern bool lan7800_isattached;

/** ペリフェラルからの読み込みの前後に必要なメモリバリア  */
#define pre_peripheral_read_mb    dmb
#define post_peripheral_read_mb   dmb

/** ペリフェラルへの書き込みの前後に必要なメモリバリア */
#define pre_peripheral_write_mb   dmb
#define post_peripheral_write_mb  dmb

/** ペリフェラルの読み書きの前後に必要なメモリバリア */
#define pre_peripheral_access_mb  dmb
#define post_peripheral_access_mb dmb

#endif /* _BCM2835_H_ */
