/**
 * @file arpInit.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <arp.h>
#include <mailbox.h>
#include <stdlib.h>
#include <thread.h>

struct arpEntry arptab[ARP_NENTRY];
mailbox arpqueue;

/**
 * @ingroup arp
 *
 * ARPテーブルとARPデーモンを初期化する。
 * @return 初期化が成功したら OK、そ例外は SYSERR
 */
syscall arpInit(void)
{
    int i = 0;

    /* ARPテーブルを初期化する */
    for (i = 0; i < ARP_NENTRY; i++)
    {
        bzero(&arptab[i], sizeof(struct arpEntry));
        arptab[i].state = ARP_FREE;
    }

    /* ARPデーモンを初期化する */
    arpqueue = mailboxAlloc(ARP_NQUEUE);
    if (SYSERR == arpqueue)
    {
        return SYSERR;
    }

    /* ARPデーモンスレッドを生成する */
    ready(create
          ((void *)arpDaemon, ARP_THR_STK, ARP_THR_PRIO, "arpDaemon", 0),
          RESCHED_NO);

    return OK;
}
