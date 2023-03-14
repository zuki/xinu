/**
 * @file telnetAlloc.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <conf.h>
#include <stddef.h>

#include <interrupt.h>
#include <telnet.h>

/**
 * @ingroup telnet
 *
 * 利用可能なtelnetデバイスを割り当てる.
 * @return telnetデバイスのデバイス番号; 利用可能なデバイスがない場合は ::SYSERR
 */
int telnetAlloc(void)
{
    irqmask im;
    int i;

    im = disable();
    for (i = 0; i < NTELNET; ++i)
    {
        if (TELNET_STATE_FREE == telnettab[i].state)
        {
            telnettab[i].state = TELNET_STATE_ALLOC;
            restore(im);
            return i + TELNET0;
        }
    }
    restore(im);

    return SYSERR;
}
