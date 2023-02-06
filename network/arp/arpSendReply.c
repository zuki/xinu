/**
 * @file arpSendReply.c
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
 * ネットワークインタフェース上でARPリクエストに対するARP応答を送信する.
 * @param pkt 応答するARPパケット
 * @return パケットを送信したら OK; そうでなければ SYSERR
 */
syscall arpSendReply(struct packet *pkt)
{
    struct netif *netptr = NULL;
    struct arpPkt *arp = NULL;
    struct netaddr dst;

    /* ポインタのエラーチェック */
    if (NULL == pkt)
    {
        return SYSERR;
    }

    ARP_TRACE("Sending ARP reply");

    /* ぽいんたを設定する */
    netptr = pkt->nif;
    arp = (struct arpPkt *)pkt->curr;

    /* ARPヘッダーを設定する */
    arp->op = hs2net(ARP_OP_REPLY);
    ARP_TRACE("Filled in op");
    memcpy(&arp->addrs[ARP_ADDR_DHA(arp)], &arp->addrs[ARP_ADDR_SHA(arp)],
           arp->hwalen);
    memcpy(&arp->addrs[ARP_ADDR_DPA(arp)], &arp->addrs[ARP_ADDR_SPA(arp)],
           arp->pralen);
    memcpy(&arp->addrs[ARP_ADDR_SHA(arp)], netptr->hwaddr.addr,
           arp->hwalen);
    memcpy(&arp->addrs[ARP_ADDR_SPA(arp)], netptr->ip.addr, arp->pralen);
    ARP_TRACE("Filled in addrs");

    dst.type = net2hs(arp->hwtype);
    dst.len = arp->hwalen;
    memcpy(dst.addr, &arp->addrs[ARP_ADDR_DHA(arp)], dst.len);

    /* パケットを宇信する */
    return netSend(pkt, &dst, NULL, ETHER_TYPE_ARP);
}
