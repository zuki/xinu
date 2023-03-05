/**
 * @file icmpSend.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <ipv4.h>
#include <icmp.h>
#include <network.h>

/**
 * @ingroup icmp
 *
 * ICMPパケットを送信する.
 *
 * @param pkt
 *      ICMPデータを含むパケット。pkt->currはICMPデータを指し示し、
 *      pkt->lenはICMPデータの長さであり、pkt->dataとpkt->currの間には
 *      少なくともICMP、IPv4、リンクの各レベルのヘッダーを置くための
 *      スペースがなければならない。
 * @param type
 *      ICMPタイプ
 * @param code
 *      ICMPコード
 * @param datalen
 *      ICMPデータペイロード長
 * @param src
 *      送信元のネットワークアドレス.  src->type == 0 の場合、送信元には
 *      自動的にパケットを送信するインタフェースのアドレスがセットされる
 * @param dst
 *      あて先のネットワークアドレス
 *
 * @return
 *      パケットの送信に成功したら ::OK; そうでなければ ::SYSERR; または、
 *      ipv4Send()により返されたエラーコード
 */
syscall icmpSend(struct packet *pkt, uchar type, uchar code,
                 uint datalen, struct netaddr *src, struct netaddr *dst)
{
    struct icmpPkt *icmp;

    /* Error check pointers */
    if (NULL == pkt)
    {
        return SYSERR;
    }

    pkt->curr -= ICMP_HEADER_LEN;
    pkt->len += ICMP_HEADER_LEN;
    icmp = (struct icmpPkt *)pkt->curr;

    icmp->type = type;
    icmp->code = code;
    icmp->chksum = 0;
    icmp->chksum = netChksum((uchar *)icmp, datalen + ICMP_HEADER_LEN);

    ICMP_TRACE("Sending ICMP packet type %d, code %d", type, code);
    return ipv4Send(pkt, src, dst, IPv4_PROTO_ICMP);
}
