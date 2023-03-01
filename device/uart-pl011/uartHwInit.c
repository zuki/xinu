/**
 * @file uartHwInit.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <uart.h>
#include <interrupt.h>
#include <clock.h>
#include "pl011.h"

#ifdef _XINU_PLATFORM_ARM_RPI_

/* Offset of UART registers from the starti of the GPIO registers. */
#define UART_GPIO_OFFSET 0x1000

#define GPFSEL1_OFFSET   0x4

/* Offset of GPPUD register from the start of the GPIO registers.  */
#define GPPUD_OFFSET     0x94

/* Offset of the GPPUDCLK0_OFFSET register from the start of the GPIO registers.
 * */
#define GPPUDCLK0_OFFSET 0x98

/* Set up the GPIO (general-purpose IO) pins needed to use the PL011 UART on the
 * Raspberry Pi.  */
static void setup_gpio_pins(void *uart_regs)
{
    void *gpio_regs = uart_regs - UART_GPIO_OFFSET;
    ulong sel;

    volatile ulong *GPFSEL1_ptr   = gpio_regs + GPFSEL1_OFFSET;
    volatile ulong *GPPUD_ptr     = gpio_regs + GPPUD_OFFSET;
    volatile ulong *GPPUDCLK0_ptr = gpio_regs + GPPUDCLK0_OFFSET;

    /* Select alternate function 0 on pins 14 and 15.  */
    sel = *GPFSEL1_ptr;
    sel &= ~(7 << 12);
    sel |= 4 << 12; /* Pin 14 */
    sel &= ~(7 << 15);
    sel |= 4 << 15; /* Pin 15 */
    *GPFSEL1_ptr = sel;

    /* Remove pull-up or pull-down on GPIO pins 14 and 15, which will be used
     * by the UART.  */
    *GPPUD_ptr = 0;
    udelay(2);
    *GPPUDCLK0_ptr = (1 << 14) | (1 << 15);
    udelay(2);
    *GPPUDCLK0_ptr = 0;
}
#endif /* _XINU_PLATFORM_ARM_RPI_ */

#ifdef _XINU_PLATFORM_ARM_RPI_3_

#include <rpi_gpio.h>
#include <bcm2837.h>

static void setup_gpio_pins(void)
{
    volatile struct rpi_gpio_regs *regptr =
        (volatile struct rpi_gpio_regs *)(GPIO_REGS_BASE);

    /* set up pins 14 & 15 to use alt0, for uart Rx and Tx */
    regptr->gpfsel[1] &= ~((7 << 12) | (7 << 15));
    regptr->gpfsel[1] |= (4 << 12) | (4 << 15);

    /* Disable pull-up/down on pins 14 & 15 */
    regptr->gppud = 0;
    udelay(2);
    regptr->gppudclk[0] = (1 << 14) | (1 << 15);
    udelay(2);
    regptr->gppudclk[0] = 0;
}

#endif /* _XINU_PLATFORM_ARM_RPI_3_ */

devcall uartHwInit(device *devptr)
{
    volatile struct pl011_uart_csreg *regptr = devptr->csr;

    /* "制御レジスタ"に0を設定してUARTを無効にする  */
    regptr->cr = 0;

#ifdef _XINU_PLATFORM_ARM_RPI_
    /* Configure the GPIO pins on the Raspberry Pi correctly. */
    setup_gpio_pins((void*)regptr);
#endif

#ifdef _XINU_PLATFORM_ARM_RPI_3_
	setup_gpio_pins();
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
