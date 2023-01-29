/**
 * @file write.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>

/**
 * @ingroup devcalls
 *
 * デバイスにデータを書き出す
 *
 * @param descrp
 *      書き出すデバイスのインデックス to.
 * @param buffer
 *      書き出すデータバッファ
 * @param count
 *     書き出すデータのバイト数
 *
 * @return
 *      成功した場合、書き出したバイト数を返す。これは書き出し
 *      エラーのために @p count より少ない場合がある。
 *      それ以外で、デバイスインデックスが正しくなかったり、
 *      データを書き出すのに成功する前に書き出しエラーが発生
 *      した場合は ::SYSERR が返される。
 *
 * デバイスによってはこの関数は単にデータをバッファリングするだけで
 * 後で割り込み処理コードにより書き出す場合もあることに注意されたい。
 */
devcall write(int descrp, const void *buffer, uint count)
{
    device *devptr;

    if (isbaddev(descrp))
    {
        return SYSERR;
    }
    devptr = (device *)&devtab[descrp];
    return ((*devptr->write) (devptr, buffer, count));
}
