/**
 * @file kputc.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <uart.h>
#include "pl011.h"

/**
 * @ingroup uarthardware
 *
 * UARTに同期的に1文字書き出す.  ハードウェアに1文字書き出すまで
 * ブロックする。割り込みハンドラは使用しない。
 *
 * @param c
 *      書き出す文字
 * @param devptr
 *      UART用のデバイステーブルエントリへのポインタ
 *
 * @return
 *      UARTに <code>unsigned char</code> として書き出し
 *      <code>int</code> にキャストした文字
 */
syscall kputc(uchar c, device *devptr)
{
    struct uart *uartptr;
    volatile struct pl011_uart_csreg *regptr;
    uint uart_im;

    /* Get pointers to the UART and to its registers.  */
    uartptr = &uarttab[devptr->minor];
    regptr = devptr->csr;

    /* UARTの割り込み状態を保存し、UARTの割り込みを無効にする。
     * 注: ここではグローバル割り込みを無効にする必要はない。
     * UART割り込みハンドラとの競合状態を防ぐには、UART割り込みを
     * 無効にすることだけが必要である */
    uart_im = regptr->imsc;
    regptr->imsc = 0;

    /* 次の文字の送信準備ができるまで待つ  */
    while ((regptr->fr & PL011_FR_TXFF))
    {
        /* Do nothing */
    }

    /* データレジスタに書き出すことでUARTに1文字送信する */
    regptr->dr = c;

    /* 送信した1文字を足す */
    uartptr->cout++;

    /* UARTの割り込みを復元して送信した文字を返す */
    regptr->imsc = uart_im;
    return c;
}
