/**
 * @file        udpClose.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <bufpool.h>
#include <stdlib.h>
#include <udp.h>
#include <interrupt.h>

/**
 * @ingroup udpexternal
 *
 * UDPデバイスを閉じる.
 *
 * 注意: UDPデバイスを閉じる際にUDPデバイス上でudpWrite()を実行する
 * スレッドが存在してはならない。一方、スレッドがudpRead()を実行中は
 * UDPデバイスを閉じても安全であり、::SYSERR を返す。ただし、すべての
 * udpRead()スレッドが復帰する前にそのUDPデバイスを再オープンしては
 * ならない。
 *
 * @param devptr UDPデバイス用のデバイスエントリ.
 * @return
 *      UDPデバイスが成功裏に閉じたら ::OK; それ以外は ::SYSERR.
 *      現在のところ、この関数が失敗するのはUDPデバイスがオープンされて
 *      いなかった場合だけである。
 */
devcall udpClose(device *devptr)
{
    struct udp *udpptr;
    irqmask im;

    /* Get pointer to the UDP structure  */
    udpptr = &udptab[devptr->minor];

    im = disable();

    /* Make sure UDP device is actually open  */
    if (UDP_OPEN != udpptr->state)
    {
        restore(im);
        return SYSERR;
    }

    /* Free the in buffer pool */
    bfpfree(udpptr->inPool);

    /* Free the in semaphore */
    semfree(udpptr->isem);

    /* Clear the UDP structure and the device IO block */
    bzero(udpptr, sizeof(struct udp));

    /* Set device state to free */
    udpptr->state = UDP_FREE;

    restore(im);
    return OK;
}
