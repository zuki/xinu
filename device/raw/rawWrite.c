/**
 * @file     rawWrite.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <raw.h>

/**
 * @ingroup raw
 *
 * rawソケットへパケットを書き出す.
 * @param devptr RAWデバイスへのポインタ
 * @param buf 書き出すバッファ
 * @param len バッファのサイズ
 * @return 書き出したオクテット数、エラーが発生した場合は SYSERR
 */
devcall rawWrite(device *devptr, void *buf, uint len)
{
    struct raw *rawptr;

    rawptr = &rawtab[devptr->minor];
    return rawSend(rawptr, buf, len);
}
