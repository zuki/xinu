/**
 * @file uartWrite.c
 */

/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <uart.h>
#include <interrupt.h>
#include <mutex.h>

/**
 * @ingroup uartgeneric
 *
 * UARTにバッファのデータを書き出す.
 *
 * 注意事項: これは非同期で動作するため、書き出されたデータは
 * 内部バッファに保存され、実際にはまだハードウェアに書き出されて
 * いない可能性がある。実際にデータをハードウェアに書き出すのは
 * UARTドライバの下半分（割り込みハンドラ; uartInterrupt()を参照）で
 * ある。例外: UART送信機がアイドル状態の場合、uartWrite()は直接
 * ハードウェアに1バイト書き出すことができる。
 *
 * @param devptr
 *      UART用のデバイステーブルエントリへのポインタ
 * @param buf
 *      書き出すデータが置かれたバッファへのポインタ
 * @param len
 *      書き出すバイト数
 *
 * @return
 *      成功した場合、書き出したバイト数を返す。これは通常 @p len で
 *      あるが、UARTがノンブロッキングモードに設定されている場合は
 *      @p len より少ない場合がある。エラー（現在のところ、uartInit() が
 *      呼ばれていない場合のみ）の場合は SYSERR を返す。
 */
devcall uartWrite(device *devptr, const void *buf, uint len)
{
    irqmask im;
    struct uart *uartptr;
    uint count;

    /* Disable interrupts and get a pointer to the UART structure and a pointer
     * to the UART's hardware registers.  */
    im = disable();
    uartptr = &uarttab[devptr->minor];

    /* uartInit() が実行済みであることを確認する */
    if (NULL == uartptr->csr)
    {
        restore(im);
        return SYSERR;
    }

    /* バッファにある各バイトの書き出しを試みる */
    for (count = 0; count < len; count++)
    {
        /* 書き出す次のバイト */
        uchar ch = ((const uchar *)buf)[count];

        /* UART送信器ハードウェアがアイドル状態の場合、そのバイトを
         * 直接ハードウェアに書き出す。そうでなければ、そのバイトを
         * 下半分（割り込みハンドラ）用に出力バッファに置く。出力
         * バッファに空き巣ベースがない場合はこれはブロックされる。 */
        if (uartptr->oidle)
        {
            uartHwPutc(uartptr->csr, ch);
            uartptr->oidle = FALSE;
            uartptr->cout++;
        }
        else
        {
            /* UARTがノンブロッキングモードの場合、下半分（割り込み
             * ハンドラ）用の出力バッファに1バイト置くためのスペースが
             * あるか確認する。なければ、指定バイト書き出さずに早々に
             * 復帰する。 */
            if ((uartptr->oflags & UART_OFLAG_NOBLOCK) &&
                uartptr->ocount == UART_OBLEN)
            {
                break;
            }
            wait(uartptr->osema);
            mutex_acquire(uartptr->olock);
            uartptr->out[(uartptr->ostart + uartptr->ocount) % UART_OBLEN] = ch;
            uartptr->ocount++;
            mutex_release(uartptr->olock);
        }
    }

    /* Restore interrupts and return the number of bytes written.  */
    restore(im);
    return count;
}
