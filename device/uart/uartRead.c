/**
 * @file uartRead.c
 */

/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <uart.h>
#include <interrupt.h>

/**
 * @ingroup uartgeneric
 *
 * UARTからデータを読み込む.
 *
 * @param devptr
 *      UART用のデバイステーブルエントリへのポインタ
 * @param buf
 *      読み込んだデータを置くバッファへのポインタ
 * @param len
 *      読み込むデータの最大バイト数
 *
 * @return
 *      成功した場合、読み込んだバイト数を返す。これは通常 @p len で
 *      あるが、UARTがノンブロッキングモードに設定されている場合は
 *      @p len より少ない場合がある。エラー（現在のところ、uartInit() が
 *      呼ばれていない場合のみ）の場合は SYSERR を返す。
 */
devcall uartRead(device *devptr, void *buf, uint len)
{
    irqmask im;
    struct uart *uartptr;
    uint count;
    uchar c;

    /* Disable interrupts and get a pointer to the UART structure.  */
    im = disable();
    uartptr = &uarttab[devptr->minor];

    /* uartInit() が実行済みであることを確認する */
    if (NULL == uartptr->csr)
    {
        restore(im);
        return SYSERR;
    }

    /* 指定された各バイトの読み込みを試みる */
    for (count = 0; count < len; count++)
    {
        /* UARTがノンブロッキングモードの場合、下半分（割り込みハンドラ）
         * からの入力バッファに利用可能なバイトがあることを確認する。ない
         * 場合は、指定バイト読み込まずに早々に復帰する。 */
        if ((uartptr->iflags & UART_IFLAG_NOBLOCK) && uartptr->icount == 0)
        {
            break;
        }

        /* 下半分（割り込みハンドラ）からの入力バッファに少なくとも1バイト
         * 現れるまで待ち、それをバッファにセットして、削除する。 */
        wait(uartptr->isema);
        c = uartptr->in[uartptr->istart];
        ((uchar*)buf)[count] = c;
        uartptr->icount--;
        uartptr->istart = (uartptr->istart + 1) % UART_IBLEN;

        /* UARTがエコーモードの場合、そのバイトをUARTにエコーバックする */
        if (uartptr->iflags & UART_IFLAG_ECHO)
        {
            uartPutc(uartptr->dev, c);
        }
    }

    /* Restore interrupts and return the number of bytes read.  */
    restore(im);
    return count;
}
