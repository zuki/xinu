/**
 * @file     udpFreebuf.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <bufpool.h>
#include <network.h>
#include <udp.h>

/**
 * @ingroup udpinternal
 *
 * UDPパケットを解放する.
 *
 * @param 解放するUDPパケット
 * @return 成功したら OK, そうでなければ SYSERR
 */
syscall udpFreebuf(struct udpPkt *udppkt)
{
    return buffree(udppkt);
}
