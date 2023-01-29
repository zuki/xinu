/**
 * @file read.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>

/**
 * @ingroup devcalls
 *
 * デバイスからデータを読み込む
 *
 * @param descrp
 *      読み込むデバイスのインデックス
 * @param buffer
 *      データを読み込むバッファ
 * @param count
 *      読み込む最大バイト数
 *
 * @return
 *      成功した場合、読み込んだバイト数を返す。これは
 *      読み込みエラーやeof条件により @p count より少ない
 *      場合がある。
 *      それ以外で、デバイスインデックスが正しくなかったり、
 *      データを読み込むのに成功する前に読み込みエラーが発生
 *      した場合は ::SYSERR が返される。
 */
devcall read(int descrp, void *buffer, uint count)
{
    device *devptr;

    if (isbaddev(descrp))
    {
        return SYSERR;
    }
    devptr = (device *)&devtab[descrp];
    return ((*devptr->read) (devptr, buffer, count));
}
