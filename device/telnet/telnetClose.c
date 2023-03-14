/**
 * @file telnetClose.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <telnet.h>
#include <stdlib.h>

/**
 * @ingroup telnet
 *
 * telnetデバイスをクローズする
 * @param devptr TELNETデバイステーブルエントリ
 * @return telnetを正しくクローズできた場合は ::OK; それ以外は ::SYSERR
 */
devcall telnetClose(device *devptr)
{
    struct telnet *tntptr;

    tntptr = &telnettab[devptr->minor];
    bzero(tntptr, sizeof(struct telnet));
    tntptr->state = TELNET_STATE_FREE;
    return OK;
}
