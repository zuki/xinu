/**
 * @file netUp.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <conf.h>
#include <device.h>
#include <interrupt.h>
#include <network.h>
#include <route.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread.h>
#include <core.h>

static int netAlloc(void);

/**
 * @ingroup network
 *
 * 指定されたプロトコルアドレスを使ってネットワークインタフェースを開始する.
 *
 * @param descrp
 *      インタフェースをオープンするネットワークデバイスのインデックス
 * @param ip
 *      プロトコルアドレス; NULLは不可
 * @param mask
 *      プロトコルアドレスマスク; NULLは不可
 * @param gateway
 *      ゲートウェイのプロトコルアドレス, 指定しない場合は NULL. 指定しない
 *      場合は「ゲーウェイなし」と解釈される。
 *
 * @return
 *      ネットワークインタフェースが成功裏に開始されたら OK; そうでなければ SYSERR
 */
syscall
netUp(int descrp, const struct netaddr *ip, const struct netaddr *mask,
              const struct netaddr *gateway)
{
    irqmask im;
    int nif;
    struct netif *netptr;
    uint i;
    uint nthreads;
    int retval = SYSERR;

    /* 1. 引数のエラーチェック */
    if (isbaddev(descrp) || NULL == ip || NULL == mask)
    {
        NET_TRACE("Bad device, null IP, or null mask");
        goto out;
    }

    if (ip->len > NET_MAX_ALEN || mask->len > NET_MAX_ALEN ||
        (NULL != gateway && gateway->len > NET_MAX_ALEN))
    {
        NET_TRACE("IP, mask, and/or gateway are longer than maximum "
                  "network address length.");
        goto out;
    }
    im = disable();

    /* 2. 指定のデバイスですでにネットワークインタフェースが
     *    開始されていないことを確認する */
    netptr = netLookup(descrp);
    if (NULL != netptr)
    {
        NET_TRACE("Network interface is already started on underlying device.");
        goto out_restore;
    }

    /* 3. 未使用のネットワークインタフェースを選択する */
    nif = netAlloc();
    if (SYSERR == nif)
    {
        NET_TRACE("Failed to allocate a network interface.");
        goto out_restore;
    }
    netptr = &netiftab[nif];
    NET_TRACE("Starting netif %d on device %d", nif, descrp);

    /* 4. ネットワークインタフェース構造体を初期化する */
    bzero(netptr, sizeof(struct netif));
    netptr->dev = descrp;
    netptr->state = NET_ALLOC;
    netptr->mtu = control(descrp, NET_GET_MTU, 0, 0);
    netptr->linkhdrlen = control(descrp, NET_GET_LINKHDRLEN, 0, 0);
    if (SYSERR == netptr->mtu || SYSERR == netptr->linkhdrlen)
    {
        NET_TRACE("Failed to get MTU and/or link header length\n");
        goto out_free_nif;
    }

    /* 5. NICのハードウェアアドレスとハードウェアブロードキャスト
     *    アドレスを取得する */
    if ((SYSERR ==
         control(descrp, NET_GET_HWADDR, (long)&netptr->hwaddr, 0))
        || (SYSERR ==
            control(descrp, NET_GET_HWBRC, (long)&netptr->hwbrc, 0)))
    {
        NET_TRACE("Couldn't get NIC addresses");
        goto out_free_nif;
    }

    /* 6. プロトコルアドレスをセットする */
    netaddrcpy(&netptr->ip, ip);
    netaddrcpy(&netptr->mask, mask);
    if (NULL != gateway)
    {
        netaddrcpy(&netptr->gateway, gateway);
    }
    /* 次のループは、たとえば、IPv4マスクが255.255.255.0、
     * IPv4アドレスが192.168.0.50の場合、IPv4ブロードキャスト
     * アドレスが192.168.0.255となるようにする */
    netptr->ipbrc.type = netptr->ip.type;
    netptr->ipbrc.len = netptr->ip.len;
    for (i = 0; i < netptr->ip.len; i++)
    {
        netptr->ipbrc.addr[i] = netptr->ip.addr[i] & netptr->mask.addr[i];
        netptr->ipbrc.addr[i] |= ~netptr->mask.addr[i];
    }

#ifdef TRACE_NET
    char str[20];
    netaddrsprintf(str, &netptr->ip);
    NET_TRACE("\tIP %s", str);
    netaddrsprintf(str, &netptr->mask);
    NET_TRACE("\tMask %s", str);
    netaddrsprintf(str, &netptr->gateway);
    NET_TRACE("\tGateway %s", str);
    netaddrsprintf(str, &netptr->hwaddr);
    NET_TRACE("\tHw Addr %s", str);
    netaddrsprintf(str, &netptr->hwbrc);
    NET_TRACE("\tHw Broadcast %s", str);
    NET_TRACE("\tMTU %d\tLink header len %d",
              netptr->mtu, netptr->linkhdrlen);
#endif

    /* TODO: nvramからホスト名を取得する（nvramが利用可能であれば）  */

    /* 7. サブネットアドレスとゲートウェイをルートテーブルに追加する */
    rtAdd(&netptr->ip, NULL, &netptr->mask, netptr);
    if (NULL != gateway)
    {
        rtDefault(&netptr->gateway, netptr);
    }

    /*  8. このインタフェースに関係する受信スレッドを立ち上げる */
    for (i = 0; i < NET_NTHR; i++)
    {
        char thrname[DEVMAXNAME + 30];
        tid_typ tid;

        sprintf(thrname, "%srecv%02d", devtab[descrp].name, i);
        tid = create(netRecv, NET_THR_STK, NET_THR_PRIO, thrname, 1, netptr);
        if (SYSERR == tid)
        {
            /* Failed to create all receive threads; kill the ones that have
             * already been spawned.  */
            nthreads = i;
            goto out_kill_recv_threads;
        }
        netptr->recvthr[i] = tid;
        ready(tid, RESCHED_NO, CORE_ZERO);
    }

    retval = OK;
    goto out_restore;

out_kill_recv_threads:
    for (i = 0; i < nthreads; i++)
    {
        kill(netptr->recvthr[i]);
    }
out_free_nif:
    netptr->state = NET_FREE;
out_restore:
    restore(im);
out:
    return retval;
}

/**
 * 未使用のネットワークインタフェースを取得する.
 * @return 未使用のネットワークインタフェースのid,
 *         すべてのネットワークインタフェース使用済みの場合は SYSERR
 */
static int netAlloc(void)
{
#if NNETIF
    int nif = 0;

    /* Check all NNETIF slots */
    for (nif = 0; nif < NNETIF; nif++)
    {
        if (NET_FREE == netiftab[nif].state)
        {
            return nif;
        }
    }
#endif
    return SYSERR;
}
