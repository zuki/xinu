/**
 * @file telnetGetc.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <telnet.h>

/**
 * @ingroup telnet
 *
 * telnetから1文字読み込む.
 * @param devptr TELNETデバイステーブルエントリ
 * @return character read from TELNETから読み込んだ文字, それ以外は
 *          telnetRead()の結果
 */
devcall telnetGetc(device *devptr)
{
    char ch;
    int result = NULL;

    result = telnetRead(devptr, &ch, 1);

    if (1 != result)
    {
        return result;
    }

    return ch;
}
