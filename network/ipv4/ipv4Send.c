/*
 * file ipv4Send.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <ipv4.h>
#include <network.h>
#include <string.h>
#include <route.h>

/**
 * @ingroup ipv4
 *
 * @brief IPv4発信パケットを送信する.
 * @param packet 送信するパケット
 * @param src 送信元IPアドレス
 * @param dst 宛先IPアドレス
 * @param proto ipパケットのプロトコル
 * @return パケットが送信された OK; ARPリクエストがタイムアウトしたら TIMEOUT;
 * インタフェースが存在しなかったら IPv4_NO_INTERFACE; 次の転送先がわからない場合は IPv4_NO_HOP;
 * それ以外は SYSERR
 */
syscall ipv4Send(struct packet *pkt, struct netaddr *src,
                 struct netaddr *dst, uchar proto)
{
    struct rtEntry *rtptr;
    struct ipv4Pkt *ip;
    struct netaddr *nxthop;

    /* ポインタのエラーチェック */
    if ((NULL == pkt) || (NULL == dst))
    {
        IPv4_TRACE("Invalid args");
        return SYSERR;
    }
    if (dst->type != NETADDR_IPv4)
    {
        IPv4_TRACE("Invalid dst type");
        return SYSERR;
    }

    /* ルートテーブルで宛先を検索する */
    rtptr = rtLookup(dst);
    if (NULL == rtptr)
    {
        IPv4_TRACE("No route");
        return SYSERR;
    }

    /* パケットにはルートテーブルに次の転送先がある */
    pkt->nif = rtptr->nif;
    if (NULL == rtptr->gateway.type)
    {
        IPv4_TRACE("Next hop is dst");
        nxthop = dst;
    }
    else
    {
        IPv4_TRACE("Next hop is gateway");
        nxthop = &rtptr->gateway;
    }

    /* 発信パケットのヘッダーを設定する */
    pkt->len += IPv4_HDR_LEN;
    pkt->curr -= IPv4_HDR_LEN;

    ip = (struct ipv4Pkt *)pkt->curr;

    /* IPパケットヘッダーを設定する */
    ip->ver_ihl = (uchar)(IPv4_VERSION << 4);
    ip->ver_ihl += IPv4_HDR_LEN / 4;
    ip->tos = IPv4_TOS_ROUTINE;
    ip->len = hs2net(pkt->len);
    ip->id = 0;
    ip->flags_froff = 0;
    ip->ttl = IPv4_TTL;
    ip->proto = proto;
    if (NULL == src->type)
    {
        /* 送信元が指定されていない。発信ネットワークインタフェースのIPを使用する */
        memcpy(ip->src, pkt->nif->ip.addr, IPv4_ADDR_LEN);
    }
    else
    {
        /* 指定された送信元IPを使用する */
        memcpy(ip->src, src->addr, IPv4_ADDR_LEN);
    }
    memcpy(ip->dst, dst->addr, IPv4_ADDR_LEN);

    /* チェックsマウを計算する */
    ip->chksum = 0;
    ip->chksum = netChksum((uchar *)ip, IPv4_HDR_LEN);
    IPv4_TRACE("Setup IPv4 header");

    /* フラグメント化してパケットを送信する */
    return ipv4SendFrag(pkt, nxthop);
}
