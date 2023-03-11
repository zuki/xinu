/**
 * @file     udpRecv.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <string.h>
#include <interrupt.h>
#include <ipv4.h>
#include <icmp.h>
#include <udp.h>

/**
 * @ingroup udpinternal
 *
 * UDPパケットを受信してUDPデバイスの入力バッファに置く.
 * @param pkt 着信UDPパケット
 * @param src 送信元アドレス
 * @param dst あて先アドレス
 * @return UDPパケットを正しく受信できたら OK; それ以外は SYSERR
 */
syscall udpRecv(struct packet *pkt, const struct netaddr *src,
                const struct netaddr *dst)
{
    struct udpPkt *udppkt;
    struct udpPseudoHdr *pseudo;
    struct udp *udpptr;
    struct udpPkt *tpkt;
#ifdef TRACE_UDP
    char strA[20];
    char strB[20];
#endif                          /* TRACE_UDP */

    irqmask im;

    /* UDPヘッダーの開始点をポイントする */
    udppkt = (struct udpPkt *)pkt->curr;

    if (NULL == udppkt)
    {
        UDP_TRACE("Invalid UDP packet.");
        netFreebuf(pkt);
        return SYSERR;
    }

    /* オプションのチェックサムを計算する */
    if ((udppkt->chksum)
        && (0 != udpChksum(pkt, net2hs(udppkt->len), src, dst)))
    {
        UDP_TRACE("Invalid UDP checksum.");
        netFreebuf(pkt);
        return SYSERR;
    }

    /* UDPヘッダーフィールドをホストオーダに変換する */
    udppkt->srcPort = net2hs(udppkt->srcPort);
    udppkt->dstPort = net2hs(udppkt->dstPort);
    udppkt->len = net2hs(udppkt->len);

    im = disable();

    /* UDPパケット用のUDPソケット（デバイス）を探す */
    udpptr = udpDemux(udppkt->dstPort, udppkt->srcPort, dst, src);

    if (NULL == udpptr)
    {
#ifdef TRACE_UDP
        UDP_TRACE("No UDP socket found for this UDP packet.");
        netaddrsprintf(strA, src);
        netaddrsprintf(strB, dst);
        UDP_TRACE("Source: %s:%d, Destination: %s:%d", strA,
                  udppkt->srcPort, strB, udppkt->dstPort);
#endif                          /* TRACE_UDP */
        restore(im);

        // TODO: パケットバイトオーダを確認する。
        /* UDPヘッダーフィールドをネットワークオーダに変換する */
        udppkt->srcPort = hs2net(udppkt->srcPort);
        udppkt->dstPort = hs2net(udppkt->dstPort);
        udppkt->len = hs2net(udppkt->len);

        /* ICMPポート到達不能メッセージを送信する */
        icmpDestUnreach(pkt, ICMP_PORT_UNR);
        netFreebuf(pkt);
        return SYSERR;
    }
    if (udpptr->icount >= UDP_MAX_PKTS)
    {
        UDP_TRACE("UDP buffer is full. Dropping UDP packet.");
        restore(im);
        netFreebuf(pkt);
        return SYSERR;
    }

    /* "bind first"フラグをチェックして、セットされていたら
     * コネクションを更新して、フラグをクリアする */
    if (UDP_FLAG_BINDFIRST & udpptr->flags)
    {
        udpptr->remotept = udppkt->srcPort;
        netaddrcpy(&(udpptr->localip), dst);
        netaddrcpy(&(udpptr->remoteip), src);
        udpptr->flags &= ~UDP_FLAG_BINDFIRST;
    }

    /* パケットを格納するバッファを取得する */
    tpkt = udpGetbuf(udpptr);

    if (SYSERR == (int)tpkt)
    {
        UDP_TRACE("Unable to get UDP buffer from pool. Dropping packet.");
        restore(im);
        netFreebuf(pkt);
        return SYSERR;
    }

    /* パケットのデータを入力バッファのカレント位置にコピーする */
    pseudo = (struct udpPseudoHdr *)tpkt;
    memcpy(pseudo->srcIp, src->addr, IPv4_ADDR_LEN);
    memcpy(pseudo->dstIp, dst->addr, IPv4_ADDR_LEN);
    pseudo->zero = 0;
    pseudo->proto = IPv4_PROTO_UDP;
    pseudo->len = udppkt->len;

    memcpy((pseudo + 1), udppkt, udppkt->len);

    /* 一時的なUDPパケットをFIFOバッファに格納する */
    udpptr->in[(udpptr->istart + udpptr->icount) % UDP_MAX_PKTS] = tpkt;
    udpptr->icount++;

    restore(im);

    signal(udpptr->isem);

    netFreebuf(pkt);

    return OK;
}
