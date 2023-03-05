/**
 * @file icmpRedirect.c
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <ipv4.h>
#include <icmp.h>
#include <string.h>
#include <route.h>

/**
 * @ingroup icmp
 *
 * ICMPリダイレクトメッセージを作成する.
 * @param redir どこか別の場所に行くべきパケット
 * @param code  ICMPリダイレクトコード番号
 * @param route パケットの新しいゲートウェイ
 * @return パケットが送信されたら OK; それ以外は SYSERR
 */
syscall icmpRedirect(struct packet *redir, uchar code,
                     struct rtEntry *route)
{
    struct packet *pkt;
    struct ipv4Pkt *ip;
    struct netaddr dst;
    int result;
    int ihl;
    struct netaddr src;

    ICMP_TRACE("ICMP redirect, code(%d)", code);
    pkt = netGetbuf();
    if (SYSERR == (int)pkt)
    {
        ICMP_TRACE("Failed to acquire packet buffer");
        return SYSERR;
    }

    ip = (struct ipv4Pkt *)redir->nethdr;
    dst.type = NETADDR_IPv4;
    dst.len = IPv4_ADDR_LEN;
    /* Send error message back to original source.                */
    memcpy(dst.addr, ip->src, dst.len);
    ihl = (ip->ver_ihl & IPv4_IHL) * 4;

    /* RFC792によれば、メッセージには少なくとも対象となるパケットの
     * ICMP_DEF_DATALENが含まれている */
    /*  of packet in question, as per RFC 792.                    */
    pkt->len = ihl + ICMP_DEF_DATALEN;
    pkt->curr -= pkt->len;

    memcpy(pkt->curr, ip, ihl + ICMP_DEF_DATALEN);
    /* ペイロードの最初の4オクテットはゲートウェイ */
    pkt->curr -= IPv4_ADDR_LEN;
    pkt->len += IPv4_ADDR_LEN;

    memcpy(pkt->curr, route->gateway.addr, IPv4_ADDR_LEN);

    src.type = 0;
    result = icmpSend(pkt, ICMP_REDIRECT, code, pkt->len, &src, &dst);

    netFreebuf(pkt);
    return result;
}
