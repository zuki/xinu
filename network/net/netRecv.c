/**
 * @file     netRecv.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <arp.h>
#include <device.h>
#include <ethernet.h>
#include <network.h>
#include <ipv4.h>
#include <snoop.h>
#include <stdlib.h>
#include <string.h>
#include <thread.h>

/**
 * @ingroup network
 *
 * 一度に1つ着信パケットを処理する受信スレッド
 *
 * @param netptr
 *      netRecvをオープンするネットワークインタフェースデバイス
 *
 * @return
 *      このスレッドは復帰しない
 */
thread netRecv(struct netif *netptr)
{
    uint maxlen;                            /* maximum packet length */
    maxlen = netptr->linkhdrlen + netptr->mtu;
    struct packet *pkt;
    struct etherPkt *ether;
    struct netaddr dst;

    /* 着信パケットを処理する */
    while (TRUE)
    {
        int len;

        /* Get a buffer for incoming packet */
        pkt = netGetbuf();
        if (SYSERR == (int)pkt)
        {
            continue;
        }

        /* 下位のネットワークデバイスからパケットを読み込む。
         * このスレッドは読み込むパケットが現れるまで待機する。
         * 読み込むべきパケットが現れことを通知してこのスレッドを
         * 実行するように告げるのはネットワークドライバの責任である。
         */
        len = read(netptr->dev, pkt->data, maxlen);
        if (ETH_HDR_LEN > len || SYSERR == len)
        {
            netFreebuf(pkt);
            continue;
        }

        pkt->len = len;
        pkt->curr = pkt->data;
        pkt->nif = netptr;
        netptr->nin++;

        /* 着信パケットバッファ内のパケット位置をポイントする */
        pkt->linkhdr = pkt->curr;
        ether = (struct etherPkt *)pkt->curr;

        /* プロミスキャストモードならスヌープ */
        if (netptr->capture != NULL)
        {
            snoopCapture(netptr->capture, pkt);
        }

        /* 宛先ハードウェアアドレスを取得する */
        dst.type = NETADDR_ETHERNET;
        dst.len = ETH_ADDR_LEN;
        memcpy(dst.addr, ether->dst, ETH_ADDR_LEN);

#ifdef TRACE_NET
        char str[20];
        NET_TRACE("Read packet len %d", pkt->len);
        netaddrsprintf(str, &dst);
        NET_TRACE("\tPacket dst %s", str);
        NET_TRACE("\tPacket proto 0x%04X", net2hs(ether->type));
#endif

        /* パケットが自宛のものかチェックする */
        if ((netaddrequal(&dst, &netptr->hwaddr))
            || (netaddrequal(&dst, &netptr->hwbrc)))
        {
            /* カレントぽ板をネットワーク層ヘッダーに移動する */
            pkt->curr = pkt->data + netptr->linkhdrlen;

            /* パケット種別に基づいて必要な関数を呼び出す */
            switch (net2hs(ether->type))
            {
                /* IP Packet */
            case ETHER_TYPE_IPv4:
                ipv4Recv(pkt);
                netptr->nproc++;
                break;

                /* ARP Packet */
            case ETHER_TYPE_ARP:
                arpRecv(pkt);
                netptr->nproc++;
                break;

                /* 未知の種別のパケットは破棄 */
            default:
                netFreebuf(pkt);
                break;
            }

        }
        else    // 自宛でなければ破棄
        {
            netFreebuf(pkt);
        }
    }

    return SYSERR;

}
