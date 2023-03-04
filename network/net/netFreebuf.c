/**
 * @file netGetbuf.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <bufpool.h>
#include <network.h>

/**
 * @ingroup network
 *
 * パケットを格納するバッファを解放する.
 * @param ptk パケットへのポインタ
 * @return 成功したら OK, エラーが発生したら SYSERR
 */
syscall netFreebuf(struct packet *pkt)
{
    return buffree(pkt);
}
