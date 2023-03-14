/** @file snoopRead.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <snoop.h>

/**
 * @ingroup snoop
 *
 * ネットワークインタフェースからキャプチャしたパケットを返す.
 * @param cap キャプチャ構造体へのポインタ
 * @return 読み込みに成功したらパケット、それ以外は ::SYSERR
 */
struct packet *snoopRead(struct snoop *cap)
{
    struct packet *pkt;

    /* 引数のエラーチェック */
    if (NULL == cap)
    {
        return (struct packet *)SYSERR;
    }

    pkt = (struct packet *)mailboxReceive(cap->queue);
    if ((SYSERR == (int)pkt) || (NULL == pkt))
    {
        return (struct packet *)SYSERR;
    }

    return pkt;
}
