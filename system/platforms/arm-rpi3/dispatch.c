/**
 * @file dispatch.c
 *
 * このファイルには、Raspberry Piに使われているBCM2835 SoCの割り込み
 * コントローラとインタフェースするコードが含まれている。
 *
 * この「割り込みコントローラ」は、より具体的には、BCM2835上の
 * ARMプロセッサが使用する割り込みコントローラである。つまり、
 * この割り込みコントローラはARMがどのIRQを受信するかを制御する。
 * VideoCoreコプロセッサは自身へのIRQを制御する独自の（文書化されて
 * いない）方法を持っていると思われる。
 *
 * 詳細は https://xinu.cs.mu.edu/index.php/BCM2835_Interrupt_Controller を参照されたい。
 *
 * 「dispatch()は system/arch/arm/irq_handler.S にあるアセンブリ言語の
 * IRQハンドラから呼び出され、割り込みの処理に使用される。dispatch()の
 * ポイントは、実際にどの番号のIRQがペンディングされているかを把握し、
 * それぞれについて登録された割り込みハンドラを呼び出すことである。」
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <interrupt.h>
#include <kernel.h>
#include <stddef.h>
#include "bcm2837.h"

/** BCM2835割り込みコントローラレジスタのレイアウト. */
struct bcm2835_interrupt_registers {
    unsigned int IRQ_basic_pending;
    unsigned int IRQ_pending_1;
    unsigned int IRQ_pending_2;
    unsigned int FIQ_control;
    unsigned int Enable_IRQs_1;
    unsigned int Enable_IRQs_2;
    unsigned int Enable_Basic_IRQs;
    unsigned int Disable_IRQs_1;
    unsigned int Disable_IRQs_2;
    unsigned int Disable_Basic_IRQs;
};

static volatile struct bcm2835_interrupt_registers * const regs =
        (volatile struct bcm2835_interrupt_registers*)INTERRUPT_REGS_BASE;

/** GPUとARMで共用されるIRQの数. これらはIRQ_pending_1とIRQ_pending_2に
 * 現れるIRQに対応する
 */
#define BCM2835_NUM_GPU_SHARED_IRQS     64

/** ARM専用のIRQの数. これらはIRQ_basic_pendingの最初の
 * 0ビットに現れるIRQに対応する
 */
#define BCM2835_NUM_ARM_SPECIFIC_IRQS   8

/* このハードウェアのIRQの総数.  */
#define BCM2835_NUM_IRQS (BCM2835_NUM_GPU_SHARED_IRQS + BCM2835_NUM_ARM_SPECIFIC_IRQS)

/** Xinuの割り込みハンドラ関数テーブル.
 * これはIRQ番号をハンドラ関数にマッピングする配列である。
 */
interrupt_handler_t interruptVector[BCM2835_NUM_IRQS];

/** ARMで有効化されたIRQのビットビットテーブル. */
static unsigned int arm_enabled_irqs[3];

/** 指定されたIRQのハンドラ関数を呼び出す. 存在しない場合はpanic
 *  @param irq_num IRQ番号
*/
static void handle_irq(uchar irq_num)
{
    interrupt_handler_t handler = interruptVector[irq_num];
    if (handler)
    {
        (*handler)();
    }
    else
    {
        kprintf("ERROR: No handler registered for interrupt %u\r\n", irq_num);

        extern void halt(void);
        halt();
    }
}

/** IRQラインの保留ビットがセットされているかチェックする.
 *  セットされていれば、そのハンドラ関数を呼び出す。
 *
 *  @param irq_num IRQ番号
 */
static void check_irq_pending(uchar irq_num)
{
    bool handle = FALSE;

    /* IRQ番号に基づいて適切なハードウェアレジスタをチェックする  */
    if (irq_num >= 64)
    {
        if (regs->IRQ_basic_pending & (1 << (irq_num - 64)))
        {
            handle = TRUE;
        }
    }
    else if (irq_num >= 32)
    {
        if (regs->IRQ_pending_2 & (1 << (irq_num - 32)))
        {
            handle = TRUE;
        }
    }
    else
    {
        if (regs->IRQ_pending_1 & (1 << irq_num))
        {
            handle = TRUE;
        }
    }
    if (handle)
    {
        handle_irq(irq_num);
        /* 保留ビットはハンドラ関数によってデバイス固有の方法で
         * クリアされたはず。知る限り割り込みコントローラを通して
         * 直接クリアすることはできない。 */
    }
}

/* 非0のワードで最初にセットされているビットの位置を探す. */
static inline unsigned long first_set_bit(unsigned long word)
{
    return 31 - __builtin_clz(word);
}

/**
 * 保留中のすべての割り込み要求を処理する.
 *
 * BCM2835 (Raspberry Pi)では、ARMに登録されているすべての割り込みを
 * 走査して保留中であるかどうかをチェックすることで行われる。これは
 * 必ずしも最速の方法ではないが、ドキュメントが不十分なハードウェアの
 * 問題やGPUとのコンフリクトを最小限に抑えることができるはず。
 */
void dispatch(void)
{
    unsigned int i;

    for (i = 0; i < 3; i++)
    {
        unsigned int mask = arm_enabled_irqs[i];
        while (mask != 0)
        {
            unsigned int bit = first_set_bit(mask);
            mask ^= (1 << bit);     // ビットクリア
            check_irq_pending(bit + (i << 5));  // bit + 32 * i
        }
    }
}

/**
 * 割り込み要求ラインを有効にする.
 * @param irq_num
 *      有効にする割り込みのインデックス. 現在のプラットフォームで
 *      有効なインデックスでなければならない
 */
void enable_irq(irqmask irq_num)
{
    if (irq_num < 32)
    {
        regs->Enable_IRQs_1 = 1 << irq_num;
        arm_enabled_irqs[0] |= 1 << irq_num;
    }
    else if (irq_num < 64)
    {
        regs->Enable_IRQs_2 = 1 << (irq_num - 32);
        arm_enabled_irqs[1] |= 1 << (irq_num - 32);
    }
    else
    {
        regs->Enable_Basic_IRQs = 1 << (irq_num - 64);
        arm_enabled_irqs[2] |= 1 << (irq_num - 64);
    }
}

/**
 * 割り込み要求ラインを無効にする.
 * @param irq_num
 *      無効にする割り込みのインデックス. 現在のプラットフォームで
 *      有効なインデックスでなければならない
 */
void disable_irq(irqmask irq_num)
{
    if (irq_num < 32)
    {
        regs->Disable_IRQs_1 = 1 << irq_num;
        arm_enabled_irqs[0] &= ~(1 << irq_num);
    }
    else if (irq_num < 64)
    {
        regs->Disable_IRQs_2 = 1 << (irq_num - 32);
        arm_enabled_irqs[1] &= ~(1 << (irq_num - 32));
    }
    else
    {
        regs->Disable_Basic_IRQs = 1 << (irq_num - 64);
        arm_enabled_irqs[2] &= ~(1 << (irq_num - 64));
    }
}
