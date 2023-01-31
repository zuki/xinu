/**
 * @file uartInit.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <uart.h>

/**
 * @ingroup uartgeneric
 *
 * 現在のEmbedded Xinu構成で利用可能なUARTデバイスのグローバルテーブル
 */
struct uart uarttab[NUART];

/**
 * @ingroup uartgeneric
 *
 *  UARTを初期化する. xinuのUARTテーブルのエントリとハードウェア自身の
 *  初期化を含む。
 *
 * @param devptr
 *      初期化するUART用のデバイステーブルエントリへのポインタ
 *
 * @return 成功の場合は、OK; 失敗の場合は、SYSERR.
 */
devcall uartInit(device *devptr)
{
    /* XinuのUARTテーブルにおけるこのUART用のエントリを初期化する  */

    struct uart *uartptr = &uarttab[devptr->minor];

    /* 統計カウントを初期化する */
    uartptr->cout = 0;
    uartptr->cin = 0;
    uartptr->lserr = 0;
    uartptr->ovrrn = 0;
    uartptr->iirq = 0;
    uartptr->oirq = 0;

    /* 入力バッファを初期化する。スレッドが待機するセマフォの初期化を含む */
    uartptr->isema = semcreate(0);
    uartptr->iflags = 0;
    uartptr->istart = 0;
    uartptr->icount = 0;
    if (isbadsem(uartptr->isema))
    {
        return SYSERR;
    }

    /* 出力バッファを初期化する。スレッドが待機するセマフォの初期化を含む */
    uartptr->osema = semcreate(UART_OBLEN);
    uartptr->oflags = 0;
    uartptr->ostart = 0;
    uartptr->ocount = 0;
    uartptr->oidle = 1;
    if (isbadsem(uartptr->osema))
    {
        semfree(uartptr->isema);
        return SYSERR;
    }

    /* 実際のハードウェアを初期化する */
    if (OK != uartHwInit(devptr))
    {
        semfree(uartptr->isema);
        semfree(uartptr->osema);
        return SYSERR;
    }
    /*デバイスへのポインタとハードウェアレジスタを UART構造体に保存する */
    uartptr->dev = devptr;
    uartptr->csr = devptr->csr;
    return OK;
}
