/**
 * @file rtSend.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <network.h>
#include <route.h>
#include <ipv4.h>
#include <icmp.h>

/**
 * @ingroup route
 *
 * パケットのルーティングを試みる.
 * @param pkt ルーティングを行う着信パケット
 * @return ルーティングに成功したら場合は OK; それ以外は SYSERR
 */
syscall rtSend(struct packet *pkt)
{
    struct ipv4Pkt *ip;
    struct netaddr dst;
    struct rtEntry *route;
    struct netaddr *nxthop;

    /* 1. 引数のエラーチェック */
    if (NULL == pkt)
    {
        return SYSERR;
    }
    /* 2. 宛先ネットワークアドレスを作成 */
    ip = (struct ipv4Pkt *)pkt->nethdr;
    dst.type = NETADDR_IPv4;
    dst.len = IPv4_ADDR_LEN;
    memcpy(dst.addr, ip->dst, dst.len);

    /* 3. ルートテーブルを検索する */
    route = rtLookup(&dst);

    /* 4. 宛先へのルートがなかった場合は未達を通知する */
    if ((SYSERR == (ulong)route) || (NULL == (ulong)route))
    {
        RT_TRACE("Routed packet: Network unreachable.");
        icmpDestUnreach(pkt, ICMP_NET_UNR);
        return SYSERR;
    }

    /* 6. ルートのネットワークインタフェースと同じ場合はリダイレクト */
    if (route->nif == pkt->nif)
    {
        // If outgoing interface is same as incoming, send ICMP redirect
        if (NULL == route->gateway.type)
        {
            icmpRedirect(pkt, ICMP_RHST, route);
        }
        else
        {
            icmpRedirect(pkt, ICMP_RNET, route);
        }
    }

    /* 7. IPヘッダーを更新する */
    ip->ttl--;
    if (0 == ip->ttl)
    {
        /* 7-1. 生存期間が過ぎたらその旨を通知 */
        icmpTimeExceeded(pkt, ICMP_TTL_EXC);
        return SYSERR;
    }

    /* 8. チェックサムの計算 */
    ip->chksum = 0;
    ip->chksum = netChksum((uchar *)ip, IPv4_HDR_LEN);

    /* 9. パケットのネットワークインタフェースを変更 */
    pkt->nif = route->nif;

    /* 10. パケットを宛先に送るか、ゲートウェイに送るか判断する */
    if (NULL == route->gateway.type)
    {
        nxthop = &dst;
    }
    else
    {
        nxthop = &route->gateway;
    }

    /* 11. パケットを送信 */
    if (SYSERR == ipv4SendFrag(pkt, nxthop))
    {
        RT_TRACE("Routed packet: Host unreachable.");
        icmpDestUnreach(pkt, ICMP_HST_UNR);
        return SYSERR;
    }

    RT_TRACE("Routed packet was sent.");

    return OK;
}
