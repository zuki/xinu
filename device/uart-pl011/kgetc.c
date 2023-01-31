/*
 * @file kgetc.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <uart.h>
#include "pl011.h"

/**
 * @ingroup uarthardware
 *
 * UARTから同期的に1文字読み込む.  1文字利用可能になるまでブロックする。
 * 割り込みハンドラは使用しない。
 *
 * @param devptr
 *      UART用のデバイステーブルエントリへのポインタ
 *
 * @return
 *      UARTから <code>unsigned char</code> として読み込み
 *      <code>int</code> にキャストした文字
 */
syscall kgetc(device *devptr)
{
    volatile struct pl011_uart_csreg *regptr;
    struct uart *uartptr;
    uint uart_im;
    uchar c;

    /* Get pointers to the UART and to its registers.  */
    uartptr = &uarttab[devptr->minor];
    regptr = devptr->csr;

    /* UARTの割り込み状態を保存し、UARTの割り込みを無効にする。
     * 注: ここではグローバル割り込みを無効にする必要はない。
     * UART割り込みハンドラとの競合状態を防ぐには、UART割り込みを
     * 無効にすることだけが必要である */
    uart_im = regptr->imsc;
    regptr->imsc = 0;

    /* 1文字受信可能になるまで待つ  */
    while ((regptr->fr & PL011_FR_RXFE))
    {
        /* Do nothing */
    }

    /* データレジスタを読み込むことでUARTから1文字取得する */
    c = regptr->dr;

    /* 受信した1文字を足す */
    uartptr->cin++;

    /* UARTの割り込みを復元して読み込んだ文字を返す  */
    regptr->imsc = uart_im;
    return c;
}
