/**
 * @file udpInit.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <stdlib.h>
#include <udp.h>

struct udp udptab[NUDP];

/**
 * @ingroup udpexternal
 *
 * UDPデバイスを開くためのスペースを確保する.
 * @param devptr UDPデバイステーブルエントリ
 * @return OK
 */
devcall udpInit(device *devptr)
{
    struct udp *udpptr;

    udpptr = &udptab[devptr->minor];
    bzero(udpptr, sizeof(struct udp));

    return OK;
}
