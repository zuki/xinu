/**
 * @file     loopbackRead.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <loopback.h>

/**
 * @ingroup loopback
 *
 * ループバックから指定した文字数だけ読み込む.
 *
 * @param devptr
 *      読み込むループバックデバイス
 * @param buf
 *      読み込んだデータを置くバッファ
 * @param len
 *      読み込む最大文字数
 *
 * @return
 *      読み込んだ文字数を返す。 @c EOF または読み込みエラーが発生した
 *      場合、これは @p len より小さくなる場合がある。また、ループバック
 *      デバイスがオープンされていない場合は @c SYSERR を返す。
 */
devcall loopbackRead(device *devptr, void *buf, uint len)
{
    struct loopback *lbkptr = NULL;
    uint i;
    unsigned char *buffer = buf;
    int ret;

    lbkptr = &looptab[devptr->minor];

    /* Check if loopback is open */
    if (LOOP_STATE_ALLOC != lbkptr->state)
    {
        return SYSERR;
    }

    for (i = 0; i < len; i++)
    {
        ret = loopbackGetc(devptr);
        if (ret == EOF)
        {
            break;
        }
        buffer[i] = ret;
    }

    return i;
}
