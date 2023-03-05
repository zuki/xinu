/**
 * @file icmpEchoReply.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <icmp.h>
#include <network.h>
#include <ipv4.h>
#include <string.h>

/**
 * @ingroup icmp
 *
 * 指定のICMPエコー要求に答えてICMPエコー応答を送信する.
 *
 * @param pkt ICMPエコー要求パケットへのポインタ.
 *      このパケットバッファは応答の送信用に再利用される。pkt->currは
 *      ICMPヘッダの先頭をポイントし、pkt->lenはリンクレベルヘッダを含む
 *      パケット全体の長さでなければならない。これらのメンバーはこの関数に
 *      よって更新され、ICMPタイプフィールドとチェックサムが変更される。
 *      しかし、パケットの所有権は取得されないので、呼び出し元によって
 *      解放されなければならない。
 *
 * @return
 *      パケットの送信に成功したら ::OK; そうでなければ ::SYSERR; または
 *      icmpSend()により返された別のエラーコード
 */
syscall icmpEchoReply(struct packet *pkt)
{
    struct ipv4Pkt *ip;
    struct netaddr src, dst;

    ICMP_TRACE("Sending ICMP_ECHOREPLY");

    /* RFC 792: 「エコー応答メッセージを作成するには、送信元とあて先の
     * アドレスを反転させ、タイプコードを0に変更し、チェックサムを再計算
     * すればよい」  */

    /* 宛先と発信元のアドレスをIPv4ヘッダーから抽出する */
    ip = (struct ipv4Pkt *)pkt->nethdr;
    dst.type = NETADDR_IPv4;
    dst.len = IPv4_ADDR_LEN;
    memcpy(dst.addr, ip->dst, dst.len);

    src.type = NETADDR_IPv4;
    src.len = IPv4_ADDR_LEN;
    memcpy(src.addr, ip->src, src.len);

    /* Set pkt->currがICMPデータを指すようにセットし、pkt->lenにはICMP
     * のデータ長をセットする。これでicmpSend()の送信用に設定される。 */
    pkt->curr += ICMP_HEADER_LEN;
    pkt->len -= (pkt->curr - pkt->data);

    /* ICMPエコー応答を送信する  */
    return icmpSend(pkt, ICMP_ECHOREPLY, 0, pkt->len, &dst, &src);
}
