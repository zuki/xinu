/**
 * @file icmpInit.c
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <mailbox.h>
#include <icmp.h>
#include <stdlib.h>
#include <thread.h>
#include <core.h>

mailbox icmpqueue;
struct icmpEchoQueue echotab[NPINGQUEUE];

/**
 * @ingroup icmp
 *
 * ICMPデーモンを初期化する.
 * @return 初期化に成功したら OK; それ以外は SYSERR
 */
syscall icmpInit(void)
{
    int i;

    /* 1. ICMPキューを初期化する */
    icmpqueue = mailboxAlloc(ICMP_NQUEUE);
    if (SYSERR == icmpqueue)
    {
        return SYSERR;
    }

    bzero(echotab, sizeof(echotab));
    for (i = 0; i < NPINGQUEUE; i++)
    {
        echotab[i].tid = BADTID;
    }

    /* 2. ICMPデーモンを起動する */
    ready(create
          ((void *)icmpDaemon, ICMP_THR_STK, ICMP_THR_PRIO, "icmpDaemon",
           0), RESCHED_NO, CORE_ZERO);

    return OK;
}
