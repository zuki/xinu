/**
 * @file fbWrite.c
 */

/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <framebuffer.h>

/**
 * @ingroup framebuffer
 *
 * 文字バッファをフレームバッファに書き出す.
 *
 * @param devptr  フレームバッファデバイスへのポインタ
 * @param buf   書き出す文字バッファ
 * @param len   バッファから書き出す文字数
 *
 * @return
 *      書き出した文字数。書き出しエラーが発生した場合、これは @p len
 *      より小さい場合がある。文字を書き出す前にエラーが発生した場合は
 *      ::SYSERR .
 */
devcall fbWrite(device *devptr, const uchar *buf, uint len)
{
    uint count;
    int result;

    for (count = 0; count < len; count++)
    {
        result = fbPutc(devptr, buf[count]);
        if (result != buf[count])
        {
            if (count == 0)
            {
                return SYSERR;
            }
            break;
        }
    }
    return count;
}
