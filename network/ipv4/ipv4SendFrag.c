/*
 * file ipv4SendFrag.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <ipv4.h>
#include <icmp.h>
#include <network.h>
#include <stdlib.h>
#include <string.h>
#include <ethernet.h>

/**
 * @ingroup ipv4
 *
 * @brief パケットを最大転送ユニットサイズのチャンクにフラグメント化する.
 *
 * @param pkt フラグメント化するパケット
 * @return OK
 */
syscall ipv4SendFrag(struct packet *pkt, struct netaddr *nxthop)
{
    uint ihl;
    uchar *data;
    uint dRem = 0;              // フラグメント化する残りのデータ量
    ushort froff;
    ushort lastFlag;
    ushort dLen;

    // 着信パケット構造体
    struct ipv4Pkt *ip;

    // 発信パケット構造体
    struct ipv4Pkt *outip;
    struct packet *outpkt;

    IPv4_TRACE("Declaration");

    if (NULL == pkt)
    {
        return SYSERR;
    }

    // 着信パケット構造体を設定する
    ip = (struct ipv4Pkt *)pkt->curr;

    // フラグメント化なし
    if (net2hs(ip->len) <= pkt->nif->mtu)
    {
        IPv4_TRACE("NetSend");

        if (netaddrequal(&pkt->nif->ipbrc, nxthop))
        {
            IPv4_TRACE("Subnet Broadcast");
            return netSend(pkt, &NETADDR_GLOBAL_ETH_BRC,
                           nxthop, ETHER_TYPE_IPv4);
        }
        return netSend(pkt, NULL, nxthop, ETHER_TYPE_IPv4);
    }

    // ヘッダーがDFを持っていないことを確認する
    if (net2hs(ip->flags_froff) & IPv4_FLAG_DF)
    {
        IPv4_TRACE("net2hs of froff");
        /* ICMPメッセージを送信する */
        icmpDestUnreach(pkt, ICMP_FOFF_DFSET);
        return SYSERR;
    }

    // フラグメント化あり

    ihl = (ip->ver_ihl & IPv4_IHL) * 4;
    data = ((uchar *)ip) + ihl;
    dRem = net2hs(ip->len) - ihl;
    froff = net2hs(ip->flags_froff) & IPv4_FROFF;
    lastFlag = net2hs(ip->flags_froff) & IPv4_FLAGS;

    // このパケットのデータ長は (MTU - ヘッダー長) を
    //  8バイトの倍数にもっと近く丸め下げる
    dLen = (pkt->nif->mtu - ihl) & ~0x7;

    pkt->len = ihl + dLen;
    ip->len = hs2net(pkt->len);

    // moreフラグメントフラグをセットする
    ip->flags_froff = IPv4_FLAG_MF & froff;
    ip->flags_froff = hs2net(ip->flags_froff);

    ip->chksum = 0;
    ip->chksum = netChksum((uchar *)ip, ihl);

    netSend(pkt, NULL, nxthop, ETHER_TYPE_IPv4);
    dRem -= dLen;
    data += dLen;
    froff += (dLen / 8);

    // 発信フラグメント用にスタックからメモリを取得する
    outpkt = netGetbuf();

    if (SYSERR == (int)outpkt)
    {
        IPv4_TRACE("allocating outpkt");
        return SYSERR;
    }

    // 発信パケットポインタと経数を設定する
    outpkt->curr -= pkt->nif->mtu;
    outip = (struct ipv4Pkt *)outpkt->curr;
    outpkt->nif = pkt->nif;

    memcpy(outip, ip, IPv4_HDR_LEN);

    // パケットがフラグメントできる限り繰り返す
    while (dRem > 0)
    {
        if (((dRem + 7) & ~0x7) > pkt->nif->mtu + IPv4_HDR_LEN)
        {
            dLen = (pkt->nif->mtu - IPv4_HDR_LEN) & ~0x7;
        }
        else
        {
            dLen = dRem;
        }
        // データセグメントをコピーする
        memcpy(outip->opts, data, dLen);

        // moreフラグメントフラグをセットする
        if (dLen == dRem)
        {
            outip->flags_froff = lastFlag & froff;
        }
        else
        {
            outip->flags_froff = IPv4_FLAG_MF & froff;
        }
        outip->flags_froff = hs2net(outip->flags_froff);

        // フィールドを更新する
        outip->len = hs2net(IPv4_HDR_LEN + dLen);
        outip->chksum = 0;
        outip->chksum = netChksum((uchar *)outip, IPv4_HDR_LEN);

        // 発信パケット長を更新する
        outpkt->len = net2hs(outip->len);

        // フラグメントを送信する
        netSend(outpkt, NULL, nxthop, ETHER_TYPE_IPv4);

        dRem -= dLen;
        data += dLen;
        froff += (dLen / 8);
    }

    IPv4_TRACE("freeing outpkt");
    netFreebuf(outpkt);
    return OK;
}
