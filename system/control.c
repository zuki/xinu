/**
 * @file control.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>

/**
 * @ingroup devcalls
 *
 * デバイス上でI/Oコントロールリクエストを実行する
 *
 * @param descrp
 *      デバイスのインデックス
 * @param func
 *      デバイス固有の特定のコントロール「関数」
 * @param arg1
 *      デバイス固有の特定のコントロール「関数」への第1引数
 * @param arg2
 *      デバイス固有の特定のコントロール「関数」への第2引数
 *
 * @return
 *      デバイスインデックスに適当なデバイスが対応しない場合、または、
 *      コントロール関数が認識できない場合は ::SYSERR を返す。
 *      それ以外は、リクエスト固有の値を返す。通常は、失敗の場合は
 *      ::SYSERR である。成功の場合は、::OK か リクエスト固有のデータ
 *      である。
 */
devcall control(int descrp, int func, long arg1, long arg2)
{
    device *devptr;

    if (isbaddev(descrp))
    {
        return SYSERR;
    }
    devptr = (device *)&devtab[descrp];
    return ((*devptr->control) (devptr, func, arg1, arg2));
}
