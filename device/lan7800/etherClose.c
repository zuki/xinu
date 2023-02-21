/**
 * @file etherClose.c
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <ether.h>

/**
 * @ingroup ether_lan7800
 * @brief lan7800用のetherClose() の実装.
 * ether.h にあるこの関数の文書を参照
 *
 * @param devptr デバイスへのポインタ
 * @return 常にSYSERRを返す（未実装）
 */
devcall etherClose(device *devptr)
{
    /* TODO: すべての未処理のUSBリクエストのキャンセル処理などが必要 */
    return SYSERR;
}
