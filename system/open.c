/**
 * @file open.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <stdarg.h>

/**
 * @ingroup devcalls
 *
 * デバイスをオープンして、read(), write(), putc(), getc() 操作が
 * できるようにする
 *
 * @param descrp
 *      オープンするデバイスのインデックス
 * @param ...
 *      デバイス固有のopen関数に渡す0個以上の引数
 *
 * @return
 *      成功の場合、::OK を返す。デバイスIDが不正、または、別の
 *      エラーが発生した場合は ::SYSERR を返す。一般に、デバイスが
 *      すでにオープンされている場合、デバイスドライバは少なくとも
 *      ::SYSERR を返すが、リソースの割り当てに失敗した場合やデバイス
 *      固有のエラーが発生した場合にも ::SYSERR が返される。
 */
devcall open(int descrp, ...)
{
    device *devptr;
    va_list ap;
    devcall result;

    if (isbaddev(descrp))
    {
        return SYSERR;
    }
    devptr = (device *)&devtab[descrp];
    va_start(ap, descrp);
    result = ((*devptr->open) (devptr, ap));
    va_end(ap);
    return result;
}
