/**
 * @file rawInit.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <raw.h>
#include <stdlib.h>

struct raw rawtab[NRAW];

/**
 * @ingroup raw
 *
 * rawソケット構造体を初期化する.
 * @param devptr RAWデバイステーブルエントリ
 * @return デバイスが初期化されたら OK
 */
devcall rawInit(device *devptr)
{
    struct raw *rawptr;

    rawptr = &rawtab[devptr->minor];
    bzero(rawptr, sizeof(struct raw));

    return OK;
}
