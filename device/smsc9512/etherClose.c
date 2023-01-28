/**
 * @file etherClose.c
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <ether.h>

/* smsc9512 用のetherClose() の実装;
 *  ether.h にあるこの関数の文書を参照  */
devcall etherClose(device *devptr)
{
    /* TODO: すべての未処理のUSBリクエストのキャンセル処理などが必要 */
    return SYSERR;
}
