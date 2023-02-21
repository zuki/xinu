/**
 * @file uartHwInit.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <uart.h>
#include <interrupt.h>
#include <clock.h>
#include "pl011.h"

#if defined(_XINU_PLATFORM_ARM_RPI_) || defined(_XINU_PLATFORM_ARM_RPI_3_)

/* GPIOレジスタの開始アドレスからのUARTレジスタのオフセット */
#define UART_GPIO_OFFSET 0x1000

#define GPFSEL1_OFFSET   0x4

/* GPIOレジスタの開始アドレスからのGPPUDレジスタのオフセット */
#define GPPUD_OFFSET     0x94

/* GPIOレジスタの開始アドレスからのGPPUDCLK0レジスタのオフセット */
#define GPPUDCLK0_OFFSET 0x98

/* Raspberry PiでPL011 UARTを使用するために必要なGPIOピンを設定する */
static void setup_gpio_pins(void *uart_regs)
{
    void *gpio_regs = uart_regs - UART_GPIO_OFFSET;
    ulong sel;

    volatile ulong *GPFSEL1_ptr   = gpio_regs + GPFSEL1_OFFSET;
    volatile ulong *GPPUD_ptr     = gpio_regs + GPPUD_OFFSET;
    volatile ulong *GPPUDCLK0_ptr = gpio_regs + GPPUDCLK0_OFFSET;

    /* GPIO pins 14 と 15 にALT0を選択.  */
    sel = *GPFSEL1_ptr;
    sel &= ~(7 << 12);
    sel |= 4 << 12; /* Pin 14 を ALT0 に*/
    sel &= ~(7 << 15);
    sel |= 4 << 15; /* Pin 15 を ALT0 に*/
    *GPFSEL1_ptr = sel;

    /* UARTで使用するGPIO pins 14 to 15 のプルアップ、プルダウンをすべて削除 (p.101) */
    // 1. pull-up/downを無効にセット
    *GPPUD_ptr = 0;
    // 2. 150 cycles待つ
    udelay(2);
    // 3. pin 14/15に制御信号のクロックを与える
    *GPPUDCLK0_ptr = (1 << 14) | (1 << 15);
    // 4. 150 cycles待つ
    udelay(2);
    // 5. 1ですべて無効にしたので省略
    // 6. クロックを削除する
    *GPPUDCLK0_ptr = 0;
}
#endif /* _XINU_PLATFORM_ARM_RPI_ */

devcall uartHwInit(device *devptr)
{
    volatile struct pl011_uart_csreg *regptr = devptr->csr;

    /* TODO:  この遅延がないと動かない。なぜ? */
    udelay(1500);

    /* "制御レジスタ"に0を設定してUARTを無効にする  */
    regptr->cr = 0;

#if defined(_XINU_PLATFORM_ARM_RPI_) || defined(_XINU_PLATFORM_ARM_RPI_3_)
    /* Raspberry Pi のGPIO品を正しく構成する */
    setup_gpio_pins((void*)regptr);
#endif

    /* 「フラグレジスタ」をポーリングして、UARTの送受信停止を待つ */
    while (regptr->fr & PL011_FR_BUSY)
    {
    }

    /* 「ライン制御レジスタ」でFIFOを無効にすることにより送信FIFOをフラッシュする */
    regptr->lcrh &= ~PL011_LCRH_FEN;

    /* 「割り込みクリア」レジスタに書き込んで、保留中の割り込みをクリアする。
     *  注: BCM2835 SoCのドキュメントでは、いくつかのビットが "Unsupported;
     *  write zero, read as don't care" とマークされている。これらのビットは
     *  以下ではコメントアウトされている */
    regptr->icr = (PL011_ICR_OEIC |
                   PL011_ICR_BEIC |
                   PL011_ICR_PEIC |
                   PL011_ICR_FEIC |
                   PL011_ICR_RTIC |
                   PL011_ICR_TXIC |
                   PL011_ICR_RXIC |
                   0 /* PL011_ICR_DSRMIC */ |
                   0 /* PL011_ICR_DCDMIC */ |
                   PL011_ICR_CTSMIC |
                   0 /* PL011_ICR_RIMIC */);

    /* UARTのボーレートを設定する。これは2つのレジスタ「整数ボーレート除数」と
     * 「小数ボーレート除数」に書き込むことで行う  */
    regptr->ibrd = PL011_BAUD_INT(115200);
    regptr->fbrd = PL011_BAUD_FRAC(115200);

    /* UARTの「ライン制御レジスタ」に適切な値を書き込み、パリティビットとFIFOを
     * 無効にして、8ビットのワード長に設定する  */
    regptr->lcrh = PL011_LCRH_WLEN_8BIT;

    /* UARTが層受信時にのみ割り込みを発生させるようにする */
    regptr->imsc = PL011_IMSC_RXIM | PL011_IMSC_TXIM;

    /* FIFOは今のところ外しておくことにした。FIFOのサイズは16で、受信FIFOに設定
     * できる最低のトリガレベルは1/8なので、受信FIFOに少なくとも2バイト入るまで
     * 最初の割り込みが発生しない。バイトが到着したらすぐに割り込みをかけたいから
     * である */
#if 0
    /* UART FIFOを有効にする */
    regptr->lcrh |= PL011_LCRH_FEN;

    /* 割り込みFIFOレベル選択レジスタを設定する。これは割り込みが発生するために
     * 必要な受信または送信FIFOの量を設定する */
    regptr->ifls = PL011_IFLS_RXIFLSEL_SEVENEIGHTHS | PL011_IFLS_TXIFLSEL_EIGHTH;
#endif

    /* UARTの制御レジスタに書き込み、送受信機能を有効にする  */
    regptr->cr = PL011_CR_RXE | PL011_CR_TXE | PL011_CR_UARTEN;

    /* UARTの割り込みハンドラをXINUの割り込みベクターに登録し、実際にUARTの
     * 割り込みラインを有効にする */
    interruptVector[devptr->irq] = devptr->intr;
    enable_irq(devptr->irq);
    return OK;
}
