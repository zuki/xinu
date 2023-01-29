/**
 * @file seek.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>

/**
 * XXX:  この関数は現在のところ明らかにどこにも使用されていない
 *
 * デバイスを位置づける（制御の非常に一般的な特殊ケース）
 *
 * @param descrp seekするデバイスの識別子
 * @param pos seekする位置
 * @return 成功時にはデバイスのseek位置、失敗時はSYSERR
 */
devcall seek(int descrp, uint pos)
{
    device *devptr;

    if (isbaddev(descrp))
    {
        return SYSERR;
    }
    devptr = (device *)&devtab[descrp];
    return ((*devptr->seek) (devptr, pos));
}
