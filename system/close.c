/**
 * @file close.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>

/**
 * @ingroup devcalls
 *
 * デバイスをクローズする
 *
 * @param descrp
 *      クローズするデバイスのインデックス
 *
 * @return
 *      背バイスのクローズに成功した場合は ::OK; そうでない場合は ::SYSERR
 *
 * ほとんどデバイスドライバはデバイスがオープンされていない場合、
 * ::SYSERR を返すが、それ以外は常にクローズは成功して、デバイスは
 * ::OK を返す。
 *
 * 注意事項: 使用中（read(), write()など）のスレッドがある間は、
 * 対応するドライバがそれを許している場合を除いて、デバイスをクローズ
 * してはならない。
 */
devcall close(int descrp)
{
    device *devptr;

    if (isbaddev(descrp))
    {
        return SYSERR;
    }
    devptr = (device *)&devtab[descrp];
    return ((*devptr->close) (devptr));
}
