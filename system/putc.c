/**
 * @file putc.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <stddef.h>
#include <device.h>

/**
 * @ingroup devcalls
 *
 * デバイスに1文字書き出す
 *
 * @param descrp
 *      文字を書き出すデバイスのインデックス
 * @param ch
 *      書き出す文字
 *
 * @return
 *      成功した場合、<code>unsigned char</code>として書き出した
 *      文字を @c int にキャストして返す。descrpが正しくない場合は
 *      ::SYSERR を返す。その他の失敗の場合は、呼び出したデバイス
 *      ドライバにより ::SYSERR か ::EOF を返す。
 */
devcall putc(int descrp, char ch)
{
    device *devptr;

    if (isbaddev(descrp))
    {
        return SYSERR;
    }
    devptr = (device *)&devtab[descrp];
    return ((*devptr->putc) (devptr, ch));
}
