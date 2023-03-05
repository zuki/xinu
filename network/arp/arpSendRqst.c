/**
 * @file arpSendRqst.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <arp.h>
#include <ethernet.h>
#include <network.h>

/**
 * @ingroup arp
 *
 * ネットワークインタフェース上でARPテーブルエントリに対するARPリクエストを送信する.
 * @param entry ARPテーブルエントリ
 * @return パケットが送信されたら OK; そうでなければ SYSERR
 */
syscall arpSendRqst(struct arpEntry *entry)
{
    struct netif *netptr = NULL;
    struct packet *pkt = NULL;
    struct arpPkt *arp = NULL;
    int result;

    /* ポインタのエラーチェック */
    if (NULL == entry)
    {
        return SYSERR;
    }

    ARP_TRACE("Sending ARP request");

    /* ネットワークインタフェースへのポインタを設定する */
    netptr = entry->nif;

    /* パケット用のバッファを取得する */
    pkt = netGetbuf();
    if (SYSERR == (int)pkt)
    {
        ARP_TRACE("Failed to acquire packet buffer");
        return SYSERR;
    }

    /* パケットバッファの末尾にARPヘッダーを置く */
    pkt->nif = netptr;
    pkt->len =
        ARP_CONST_HDR_LEN + netptr->hwaddr.len * 2 + netptr->ip.len * 2;
    pkt->curr -= pkt->len;
    arp = (struct arpPkt *)pkt->curr;

    /* ARPヘッダーを設定する */
    arp->hwtype = hs2net(netptr->hwaddr.type);
    arp->prtype = hs2net(netptr->ip.type);
    arp->hwalen = netptr->hwaddr.len;
    arp->pralen = netptr->ip.len;
    arp->op = hs2net(ARP_OP_RQST);
    ARP_TRACE("Filled in types, lens, and op");
    memcpy(&arp->addrs[ARP_ADDR_SHA(arp)], netptr->hwaddr.addr,
           arp->hwalen);
    memcpy(&arp->addrs[ARP_ADDR_SPA(arp)], netptr->ip.addr, arp->pralen);
    memcpy(&arp->addrs[ARP_ADDR_DPA(arp)], entry->praddr.addr,
           arp->pralen);
    ARP_TRACE("Filled in addrs");

    /* パケットを送信する */
    result = netSend(pkt, &netptr->hwbrc, NULL, ETHER_TYPE_ARP);

    ARP_TRACE("Sent packet");

    /* パケット用のバッファを解放する */
    if (SYSERR == netFreebuf(pkt))
    {
        ARP_TRACE("Failed to free packet buffer");
        return SYSERR;
    }

    /* netSendの結果を返す */
    return result;
}
