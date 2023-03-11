/**
 * @file     udpOpen.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <stddef.h>
#include <stdlib.h>
#include <bufpool.h>
#include <device.h>
#include <network.h>
#include <udp.h>
#include <stdarg.h>
#include <interrupt.h>

static ushort allocPort(void);

/**
 * @ingroup udpexternal
 *
 * UDPをオープンする. UDPソケットにローカルIPアドレス、
 * リモートIPアドレス、ポートを関連付け、udpRead()とudpWrite()で
 * データの送受信受信する準備をする。
 *
 * @param devptr UDPデバイス用のデバイステーブルエントリ
 *
 * @param ap 4つの追加引数で、以下をこの順に指定する:
 *     - ローカルIPアドレス.
 *     - リモートIPアドレスアドレス.  はじめにバインドなしのソケットを
 *       作成する場合は @c NULL.
 *     - ローカルポート.  ポート番号を自動付与する場合は 0.
 *     - リモートポート.  はじめにバインドなしのソケットを
 *       作成する場合は 0.
 *
 * @return UDPデバイスのオープンに成功した場合は ::OK; それ以外は ::SYSERR.
 */
devcall udpOpen(device *devptr, va_list ap)
{
    irqmask im;
    int retval;
    struct udp *udpptr;
    const struct netaddr *localip;
    const struct netaddr *remoteip;
    ushort localpt;
    ushort remotept;

    udpptr = &udptab[devptr->minor];

    im = disable();
    /* UDPがすでにオープンされていないかチェックする */
    if (UDP_OPEN == udpptr->state)
    {
        UDP_TRACE("udp%d has already been opened.", devptr->minor);
        retval = SYSERR;
        goto out_restore;
    }

    udpptr->state = UDP_OPEN;
    udpptr->dev = devptr;

    /* 着信パケットバッファを初期化する */
    udpptr->icount = 0;
    udpptr->istart = 0;

    /* セマフォを初期化する */
    udpptr->isem = semcreate(0);

    if (SYSERR == (int)udpptr->isem)
    {
        retval = SYSERR;
        goto out_udp_close;
    }

    /* 引数からポートとアドレスを抽出する */
    localip = va_arg(ap, const struct netaddr *);
    remoteip = va_arg(ap, const struct netaddr *);
    localpt = va_arg(ap, int);
    remotept = va_arg(ap, int);

    /* ポートとアドレスを初期化する */

    /* ローカルIPアドレスは必須 */
    if (NULL == localip)
    {
        retval = SYSERR;
        goto out_free_sem;
    }

    netaddrcpy(&udpptr->localip, localip);

    /* リモートIPアドレスは必須ではない */
    if (NULL == remoteip)
    {
        bzero(&udpptr->remoteip, sizeof(struct netaddr));
    }
    else
    {
        netaddrcpy(&udpptr->remoteip, remoteip);
    }

    /* ローカルポートが指定されていなかった場合は割り当てる */
    if (0 == localpt)
    {
        localpt = allocPort();
    }

    udpptr->localpt = localpt;
    udpptr->remotept = remotept;

    /* 受信用のUDPパケットバッファプールを割り当てる */
    udpptr->inPool = bfpalloc(NET_MAX_PKTLEN, UDP_MAX_PKTS);
    if (SYSERR == (int)udpptr->inPool)
    {
        retval = SYSERR;
        goto out_release_port;
    }
    UDP_TRACE("udp%d inPool has been assigned pool ID %d.\r\n",
              devptr->minor, udpptr->inPool);

    udpptr->flags = 0;

    retval = OK;
    goto out_restore;

out_release_port:
    udpptr->localpt = 0;
out_free_sem:
    semfree(udpptr->isem);
out_udp_close:
    udpptr->state = UDP_FREE;
out_restore:
    restore(im);
    return retval;
}

/**
 * ローカルポートとして使用する未使用のUDPポートを割り当てる
 * @return UDPポート
 */
static ushort allocPort(void)
{
    static ushort nextport = 0;
    int i = 0;

    nextport = (nextport + 1) % (UDP_PMAX - UDP_PSTART);
    for (i = 0; i < NUDP; i++)
    {
        if ((udptab[i].localpt == (nextport + UDP_PSTART))
            || (UDP_PORT_TRACEROUTE == (nextport + UDP_PSTART)))
        {
            nextport = (nextport + 1) % (UDP_PMAX - UDP_PSTART);
            i = 0;
        }
    }

    return (nextport + UDP_PSTART);
}
