/**
 * @file netDown.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <interrupt.h>
#include <network.h>
#include <route.h>
#include <thread.h>

/**
 * @ingroup network
 *
 * ネットワークインタフェースを切断する.
 *
 * @param descrp
 *      インタフェースの.
 *
 * @return
 *      インタフェースを成功裏に切断できたら ::OK; そうでなければ ::SYSERR.
 *      現在のところ、::SYSERR が返されるのはそのデバイスで実行中のネットワーク
 *      ネットワークがない場合のみ
 */
syscall netDown(int descrp)
{
    struct netif *netptr;
    irqmask im;
    uint i;

    im = disable();

    /* Determine which network interface is running on the underlying device.
     * Return SYSERR if there is none.  */
    netptr = netLookup(descrp);
    if (NULL == netptr)
    {
        restore(im);
        return SYSERR;
    }

    NET_TRACE("Stopping netif %u on device %d", netptr - netiftab, descrp);

    /* 受信スレッドをKillする。TODO: ここには既知のバグがある:
     * 不適切なタイミングで受信スレッドをkillし、（netGetbuf()で割り当てた
     * パケットバッファなどの）リソースがリークする可能性がある。  */
    for (i = 0; i < NET_NTHR; i++)
    {
        kill(netptr->recvthr[i]);
    }

    /* このネットワークインタフェースに関するルートテーブルのすべての
     * エントリをクリアする */
    rtClear(netptr);

    /* インタフェースに空きマークを付け、割り込みを復元して成功を返す  */
    netptr->state = NET_FREE;
    restore(im);
    return OK;
}
