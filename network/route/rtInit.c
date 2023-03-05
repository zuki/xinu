/**
 * @file rtInit.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <mailbox.h>
#include <network.h>
#include <route.h>
#include <stdlib.h>
#include <thread.h>
#include <core.h>

/** @ingroup route
 * @var rttab[RT_NENTRY]
 * @brief ルートテーブル.
 * デフォルトルートとして少なくともエントリを1つ含む必要がある.
 */
struct rtEntry rttab[RT_NENTRY];
/** @ingroup route
 * @var rtqueue
 * @brief ルートキューメールボックス
 */
mailbox rtqueue;

/**
 * @ingroup route
 *
 * ルートテーブルとルートデーモンを初期化する.
 * @return 初期化に成功したら OK; そうでなければ SYSERR
 */
syscall rtInit(void)
{
    int i = 0;

    /* 1. ルートテーブルを初期化する */
    for (i = 0; i < RT_NENTRY; i++)
    {
        bzero(&rttab[i], sizeof(struct rtEntry));
        rttab[i].state = RT_FREE;
    }

    /* 2. ルートキューを初期化する */
    rtqueue = mailboxAlloc(RT_NQUEUE);
    if (SYSERR == rtqueue)
    {
        return SYSERR;
    }

    /* 3. ルートデーモンを起動する */
    ready(create
          ((void *)rtDaemon, RT_THR_STK, RT_THR_PRIO, "rtDaemon", 0),
          RESCHED_NO, CORE_ZERO);

    return OK;
}
