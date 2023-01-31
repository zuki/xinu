/**
 * @file uartControl.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <uart.h>
#include <device.h>

/**
 * @ingroup uartgeneric
 *
 * UARTのパラメタを制御する.
 *
 * @param devptr  UARTデバイスへのポインタ
 * @param func  実行する関数のインデックス（uart.hで定義）
 * @param arg1  呼び出される関数への第1引数
 * @param arg2  呼び出される関数への第2引数
 *
 * @return
 *      制御関数が認識できない場合は、SYSERR; そうでなければ
 *      制御関数依存の値。
 */
devcall uartControl(device *devptr, int func, long arg1, long arg2)
{
    struct uart *uartptr;
    char old;

    uartptr = &uarttab[devptr->minor];

    switch (func)
    {

        /* 入力モードフラグをセットする: arg1 = セットするフラグ */
        /* return = 変更前のフラグの値 */
    case UART_CTRL_SET_IFLAG:
        old = uartptr->iflags & arg1;
        uartptr->iflags |= arg1;
        return old;

        /* 入力モードフラグをクリアする: arg1 = クリアするフラグ */
        /* return = クリアする前のフラグの値 */
    case UART_CTRL_CLR_IFLAG:
        old = uartptr->iflags & arg1;
        uartptr->iflags &= ~(arg1);
        return old;

        /* 入力フラグを取得する: return = フラグの現在の値 */
    case UART_CTRL_GET_IFLAG:
        return uartptr->iflags;

        /* 出力モードフラグをセットする: arg1 = セットするフラグ */
        /*  return = 変更前のフラグの値 */
    case UART_CTRL_SET_OFLAG:
        old = uartptr->oflags & arg1;
        uartptr->oflags |= arg1;
        return old;

        /* 出力モードフラグをクリアする: arg1 = クリアするフラグ */
        /* return = クリアする前のフラグの値 */
    case UART_CTRL_CLR_OFLAG:
        old = uartptr->oflags & arg1;
        uartptr->oflags &= ~(arg1);
        return old;

        /* 出力フラグを取得する: return = フラグの現在の値 */
    case UART_CTRL_GET_OFLAG:
        return uartptr->oflags;

        /* UART送信器がアイドル状態であるかチェックし、アイドルならTRUEを返す */
    case UART_CTRL_OUTPUT_IDLE:
        return uartptr->oidle;

    }
    return SYSERR;
}
