/**
 * @file udpAlloc.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <interrupt.h>
#include <udp.h>

/**
 * @ingroup udpexternal
 *
 * 利用可能なUDPデバイスを割り当てる.
 *
 * @return
 *      UDPデバイスのデバイス番号, 利用可能なデバイスがない場合は ::SYSERR.
 */
ushort udpAlloc(void)
{
    irqmask im;
    int i;

    im = disable();
    for (i = 0; i < NUDP; i++)
    {
        if (UDP_FREE == udptab[i].state)
        {
            udptab[i].state = UDP_ALLOC;
            restore(im);
            return i + UDP0;
        }
    }
    restore(im);

    return SYSERR;
}
