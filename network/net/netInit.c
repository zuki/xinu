/* @file netInit.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <arp.h>
#include <icmp.h>
#include <bufpool.h>
#include <network.h>
#include <route.h>
#include <stdlib.h>
#include <stdio.h>
#include <tcp.h>
#include <core.h>

#ifndef NNETIF
#define NNETIF 0
#endif

/**
 * @ingroup network
 * ネットワークインタフェーステーブル変数
*/
struct netif netiftab[NNETIF];
/**
 * @ingroup network
 * パケットバッファプール変数
*/
int netpool;

/**
 * @ingroup network
 *
 * ネットワークインタフェースを初期化する.
 * @return 正しく初期化されたら OK, そうでなければ SYSERR
 */
syscall netInit()
{
    int i = 0;

    /* 1. ネットワークインタフェーステーブルをクリアする */
    for (i = 0; i < NNETIF; i++)
    {
        bzero(&netiftab[i], sizeof(struct netif));
        netiftab[i].state = NET_FREE;
    }

    /* 2. パケットバッファプールを割り当てる */
    netpool = bfpalloc(NET_MAX_PKTLEN + sizeof(struct packet),
                       NET_POOLSIZE);
    NET_TRACE("netpool has been assigned pool ID %d.\r\n", netpool);
    if (SYSERR == netpool)
    {
        return SYSERR;
    }

    /* 3. ARPを初期化する */
    if (SYSERR == arpInit())
    {
        return SYSERR;
    }

    /* 4. ルートテーブルを初期化する */
    if (SYSERR == rtInit())
    {
        return SYSERR;
    }

    /* 5. ICMPを初期化する */
    if (SYSERR == icmpInit())
    {
        return SYSERR;
    }

    /* 6. TCPを初期化する:  tcptimerプロセスを実行する */
#if NTCP
    i = create((void *)tcpTimer, INITSTK, INITPRIO, "tcpTimer", 0);
    if (SYSERR == i)
    {
        return SYSERR;
    }
    else
    {
        ready(i, RESCHED_NO, CORE_ZERO);
    }
#endif                          /* NTCP */

    return OK;
}
