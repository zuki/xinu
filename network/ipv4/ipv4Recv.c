/**
 * @file ipv4Recv.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <ipv4.h>
#include <network.h>
#include <raw.h>
#include <route.h>
#include <udp.h>
#include <tcp.h>
#include <icmp.h>
#include <netemu.h>

/**
 * @ingroup ipv4
 *
 * 着信IPv4パケットを処理する.
 * @param pkt 着信パケットへのポインタ
 * @return パケットの処理に成功したら OK; それ以外は SYSERR
 */
syscall ipv4Recv(struct packet *pkt)
{
    struct ipv4Pkt *ip;
    struct netaddr dst;
    struct netaddr src;
    ushort iplen;

    /* ポインタのエラーチェック */
    if (NULL == pkt)
    {
        return SYSERR;
    }

    /* ポインタをIPv4ヘッダーに設定する */
    pkt->nethdr = pkt->curr;
    ip = (struct ipv4Pkt *)pkt->curr;

    /* IPパケットが有効であることを確認する */
    if (FALSE == ipv4RecvValid(ip))
    {
        IPv4_TRACE("Invalid packet");
        netFreebuf(pkt);
        return SYSERR;
    }

    /* 宛先と発信元のIPアドレスを取得する */
    dst.type = NETADDR_IPv4;
    dst.len = IPv4_ADDR_LEN;
    memcpy(dst.addr, ip->dst, dst.len);
    src.type = NETADDR_IPv4;
    src.len = IPv4_ADDR_LEN;
    memcpy(src.addr, ip->src, src.len);

    /* パケットがこのネットワークインタフェースに向けられたものでない
     * 場合はパケットの転送を試みる */
    if (FALSE == ipv4RecvDemux(&dst))
    {
        IPv4_TRACE("Packet sent to routing subsystem");

#if NETEMU
        /* 有効になっていればパケットをネットワークエミュレータ
         * 経由で実行する */
        return netemu(pkt);
#else
        return rtRecv(pkt);
#endif
    }

    /* パケットがフラグメント化されているかチェックする（未対応） */
    if ((IPv4_FLAG_MF & net2hs(ip->flags_froff))
        || (0 != (net2hs(ip->flags_froff) & IPv4_FROFF)))
    {
        // TODO: Handle fragmenting of packet
        // TODO: Should send icmp time exceeded
        // return ipRecvSendFrag(pkt, &dst);
        IPv4_TRACE("Packet fragmented");
        netFreebuf(pkt);
        return SYSERR;
    }

    /* Ethernetドライバは60バイト未満のパケットをパディングする。
     * Ethernetドライバから返されたパケット長（pkt->len）がパケット
     * ヘッダと一致しない場合、パケット長を調節してパディングをサック所
     * する。 */
    iplen = net2hs(ip->len);
    if ((pkt->len - pkt->nif->linkhdrlen) > iplen)
    {
        pkt->len = pkt->nif->linkhdrlen + iplen;
    }

    /* カレントポインタをアプリケーションレベルヘッダに移動する */
    pkt->curr += ((ip->ver_ihl & IPv4_IHL) << 2);

    IPv4_TRACE("IPv4 proto %d", ip->proto);
    /* パケットプロトコルで切り分ける */
    switch (ip->proto)
    {
        /* ICMP パケット */
    case IPv4_PROTO_ICMP:
        icmpRecv(pkt);
        break;

#if NUDP
        /* UDP パケット */
    case IPv4_PROTO_UDP:
        udpRecv(pkt, &src, &dst);
        break;
#endif

#if NTCP
        /* TCP パケット */
    case IPv4_PROTO_TCP:
        tcpRecv(pkt, &src, &dst);
        break;
#endif

        /* 未知のIPパケットプロトコル */
    default:
#if NRAW
        rawRecv(pkt, &src, &dst, ip->proto);
#else
        netFreebuf(pkt);
#endif
        break;
    }

    return OK;
}
