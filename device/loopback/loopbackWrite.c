/**
 * @file     loopbackWrite.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <loopback.h>

/**
 * @ingroup loopback
 *
 * ループバックデバイスにデータを書き出す.
 *
 * @param devptr
 *      データを書き出すループバックデバイスへのポインタ
 * @param buf
 *      書き出すデータを格納したバッファ
 * @param len
 *      書き出すデータのバイト数
 *
 * @return
 *      成功の場合、書き出したバイト数を返す。書き出しエラーが発生した
 *      場合、これは @p len 未満になる場合がある。また、ループバック
 *      デバイスがオープンされていない、または、データが1文字も書き出す
 *      前にエラーが発生した場合は @c SYSERR を返す。
 */
devcall loopbackWrite(device *devptr, const void *buf, uint len)
{
    struct loopback *lbkptr = NULL;
    uint i;
    const uchar *buffer = buf;

    lbkptr = &looptab[devptr->minor];

    /* Check if loopback is open */
    if (LOOP_STATE_ALLOC != lbkptr->state)
    {
        return SYSERR;
    }

    for (i = 0; i < len; i++)
    {
        if (SYSERR == loopbackPutc(devptr, buffer[i]))
        {
            if (i == 0)
            {
                i = SYSERR;
            }
            break;
        }
    }

    return i;
}
