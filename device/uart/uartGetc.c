/**
 * @file uartGetc.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <device.h>
#include <stddef.h>
#include <uart.h>

/**
 * @ingroup uartgeneric
 *
 * UARTから1文字読み込む.
 *
 * @param devptr
 *      UART用のデバイステーブルエントリへのポインタ
 *
 * @return
 *      成功した場合、<code>unsigned char</code>として読み込んだ文字を
 *      <code>int</code> にキャストして返す。読み込みエラー（不正な
 *      デバイス、またはeof）の場合は SYSERR を返す。
 */
devcall uartGetc(device *devptr)
{
    uchar ch;
    int retval;

    retval = uartRead(devptr, &ch, 1);
    if (retval == 1)
    {
        return (int)ch;
    }
    else
    {
        return SYSERR;
    }
}
