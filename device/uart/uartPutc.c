/**
 * @file uartPutc.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <device.h>
#include <stddef.h>
#include <uart.h>

/**
 * @ingroup uartgeneric
 *
 * UARTに1文字書き出す.
 *
 * @param devptr
 *      UART用のデバイステーブルエントリへのポインタ
 * @param ch
 *      書き出す文字
 *
 * @return
 *      成功した場合、<code>unsigned char</code> として書き出した文字を
 *      @c int にキャストして返す。失敗の場合は SYSERR を返す。
 */
devcall uartPutc(device *devptr, char ch)
{
    int retval;

    retval = uartWrite(devptr, &ch, 1);
    if (retval == 1)
    {
        return (int)ch;
    }
    else
    {
        return SYSERR;
    }
}
