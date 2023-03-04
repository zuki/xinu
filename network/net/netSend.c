/**
 * @file netSend.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <arp.h>
#include <device.h>
#include <ethernet.h>
#include <network.h>
#include <snoop.h>
#include <string.h>

/**
 * @ingroup network
 *
 * リンク層のヘッダーをパケットに追加してインタフェースに書き出す.
 * @param pkt 送信するパケット
 * @param hwaddr 宛先のハードウェアアドレス, 検索が必要な場合は NULL
 * @param praddr 宛先のプロトコルアドレス, hwaddrが基地の場合は NULL
 * @param type リンク層ヘッダーに埋め込むパケットの種別
 * @return パケットが送信されたら OK, ARPリクエストがタイムアウトした場合は
 *         TIMEOUT, それ以外は SYSERR
 */
syscall netSend(struct packet *pkt, const struct netaddr *hwaddr,
                const struct netaddr *praddr, ushort type)
{
    struct netif *netptr = NULL;        /* pointer to network interface */
    struct etherPkt *ether = NULL;      /* pointer to Ethernet header   */
    int result;                         /* result of ARP lookup         */
    struct netaddr addr;

    /* 1. エラーチェック */
    if (NULL == pkt)
    {
        return SYSERR;
    }
    netptr = pkt->nif;
    if ((NULL == netptr) || (netptr->state != NET_ALLOC))
    {
        return SYSERR;
    }

    NET_TRACE("Send packet of type 0x%04X", type);

    /* 2. リンク層ヘッダー用のスペースを確保する */
    pkt->curr -= netptr->linkhdrlen;
    pkt->len += netptr->linkhdrlen;
    ether = (struct etherPkt *)(pkt->curr);

    /* 3. Ethernetヘッダーを設定する */
    ether->type = hs2net(type);
    memcpy(ether->src, netptr->hwaddr.addr, netptr->hwaddr.len);
#ifdef TRACE_NET
    char str[20];
    netaddrsprintf(str, &netptr->hwaddr);
    NET_TRACE("Src = %s", str);
#endif

    /* 4. ハードウェアアドレスの指定がなかった場合はプロトコルアドレスを使って
     *    検索する */
    if (NULL == hwaddr)
    {
        NET_TRACE("Hardware address lookup required");
        hwaddr = &addr;
        result = arpLookup(netptr, praddr, (struct netaddr*)hwaddr);
        if (result != OK)
        {
            return result;
        }
    }

    /* 5. 宛先ハードウェアアドレスをethernetヘッダーにコピーする */
    memcpy(ether->dst, hwaddr->addr, hwaddr->len);

    /* 6. デバイスにパケットを書き出す */
    if (pkt->len != write(netptr->dev, pkt->curr, pkt->len))
    {
        return SYSERR;
    }

    /* 6. パケットをSnoopする */
    if (netptr->capture != NULL)
    {
        snoopCapture(netptr->capture, pkt);
    }

    return OK;
}
