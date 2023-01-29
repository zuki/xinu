/**
 * @file getc.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <stddef.h>
#include <device.h>

/**
 * @ingroup devcalls
 *
 * デバイスから1文字読み込む
 *
 * @param descrp
 *      文字を読み込むデバイスのインデックス
 *
 * @return
 *      成功した場合、読み込んだ文字を <code>unsigned char</code> から
 *      @c int にキャストして返す。descrpが正しくない場合は
 *      ::SYSERR を返す。その他の失敗の場合は、呼び出したデバイス
 *      ドライバにより ::SYSERR か ::EOF を返す。
 */
devcall getc(int descrp)
{
    device *devptr;

    if (isbaddev(descrp))
    {
        return SYSERR;
    }
    devptr = (device *)&devtab[descrp];
    return ((*devptr->getc) (devptr));
}
