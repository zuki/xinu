/**
 * @file telnetPutc.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <telnet.h>
#include <device.h>

/**
 * @ingroup telnet
 *
 * telnetに1文字書き出す.
 *
 * @param devptr
 *      TELNETデバイステーブルエントリ
 * @param ch
 *      書き出す文字
 *
 * @return
 *      成功の場合は @p ch を <code>unsigned char</code> から @c int に
 *      キャストして返す; 失敗の場合は ::SYSERR.
 */
devcall telnetPutc(device *devptr, char ch)
{
    int ret;

    ret = telnetWrite(devptr, &ch, 1);
    if (ret == 1)
    {
        return (uchar)ch;
    }
    else
    {
        return SYSERR;
    }
}
