/** @file snoopFilter.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <arp.h>
#include <ipv4.h>
#include <network.h>
#include <snoop.h>
#include <string.h>

/**
 * @ingroup snoop
 *
 * パケットがフィルタにマッチするか判定する.
 * @param s スヌープ構造体へのポインタ
 * @param pkt パケットへのポインタ
 * @return パケットがフィルタにマッチしたら ::TRUE; それ以外は ::FALSE
 */
bool snoopFilter(struct snoop *s, struct packet *pkt)
{
    struct etherPkt *ether = NULL;
    struct arpPkt *arp = NULL;
    struct ipv4Pkt *ip = NULL;

    /* フィルタがない場合はパケットはフィルタにマッチしたとする */
    if ((NULL == s->type)
        && (NULL == s->srcaddr.type) && (NULL == s->srcport)
        && (NULL == s->dstaddr.type) && (NULL == s->dstport))
    {
        return TRUE;
    }

    /* パケットのサイズが妥当であること。そうでなければフィルタリングは
       できない */
    if (pkt->len < ETH_HDR_LEN)
    {
        /* パケットはEthernetヘッダを含むだけの長さがない */
        return FALSE;
    }
    else
    {
        ether = (struct etherPkt *)pkt->curr;
        switch (net2hs(ether->type))
        {
        case ETHER_TYPE_ARP:
            if (pkt->len < (ETH_HDR_LEN + ARP_CONST_HDR_LEN))
            {
                /* パケットはARPヘッダを含むだけの長さがない */
                return FALSE;
            }
            arp = (struct arpPkt *)ether->data;
            if (pkt->len < (ETH_HDR_LEN + ARP_CONST_HDR_LEN +
                            (2 * arp->hwalen) + (2 * arp->pralen)))
            {
                /* パケットはARPヘッダを含むだけの長さがない */
                return FALSE;
            }
            break;
        case ETHER_TYPE_IPv4:
            if (pkt->len < (ETH_HDR_LEN + IPv4_HDR_LEN))
            {
                /* パケットはIPv4ヘッダを含むだけの長さがない */
                return FALSE;
            }
            ip = (struct ipv4Pkt *)ether->data;
            break;
        default:
            /* その他のEthernetタイプはフィルタリングする方法を知らない */
            return FALSE;
        }
    }

    /* ここまで到達したということはパケットは妥当な長さである */

    /* パケットタイプがマッチするかチェックする */
    if (s->type != NULL)
    {
        /* Check Ethernet type */
        switch (net2hs(ether->type))
        {
        case ETHER_TYPE_ARP:
            if (s->type != SNOOP_FILTER_ARP)
            {
                return FALSE;
            }
            break;
        case ETHER_TYPE_IPv4:
            if (s->type != SNOOP_FILTER_IPv4)
            {
                /* Check IP protocol */
                switch (ip->proto)
                {
                case IPv4_PROTO_UDP:
                    if (s->type != SNOOP_FILTER_UDP)
                    {
                        return FALSE;
                    }
                    break;
                case IPv4_PROTO_TCP:
                    if (s->type != SNOOP_FILTER_TCP)
                    {
                        return FALSE;
                    }
                    break;
                case IPv4_PROTO_ICMP:
                    if (s->type != SNOOP_FILTER_ICMP)
                    {
                        return FALSE;
                    }
                    break;
                default:
                    return FALSE;
                }
            }
            break;
        default:
            return FALSE;
        }
    }

    /* ここまで到達したということはフィルタがないか、パケットが
     * タイプフィルタにマッチした */

    /* パケットの送信元アドレスがマッチするかチェックする */
    if (s->srcaddr.type != NULL)
    {
        /* ARP */
        if (arp != NULL)
        {
            if ((s->srcaddr.len != arp->pralen)
                || (0 != memcmp(&arp->addrs[ARP_ADDR_SPA(arp)],
                                s->srcaddr.addr, arp->pralen)))
            {
                return FALSE;
            }
        }
        /* IPv4 */
        else if (ip != NULL)
        {
            if ((s->srcaddr.len != IPv4_ADDR_LEN)
                || (0 != memcmp(ip->src, s->srcaddr.addr, IPv4_ADDR_LEN)))
            {
                return FALSE;
            }
        }
        /* Unknown */
        else
        {
            return FALSE;
        }
    }

    /* パケットのあて先アドレスがマッチするかチェックする */
    if (s->dstaddr.type != NULL)
    {
        /* ARP */
        if (arp != NULL)
        {
            if ((s->dstaddr.len != arp->pralen)
                || (0 != memcmp(&arp->addrs[ARP_ADDR_DPA(arp)],
                                s->dstaddr.addr, arp->pralen)))
            {
                return FALSE;
            }
        }
        /* IPv4 */
        else if (ip != NULL)
        {
            if ((s->dstaddr.len != IPv4_ADDR_LEN)
                || (0 != memcmp(ip->dst, s->dstaddr.addr, IPv4_ADDR_LEN)))
            {
                return FALSE;
            }
        }
        /* Unknown */
        else
        {
            return FALSE;
        }
    }

    /* パケットポートがマッチするかチェックする */
    /* FINME: snoopにポートによるフィルタリング指定があるとsnoopできない */
    if ((s->srcport != NULL) || (s->dstport != NULL))
    {
        if (ip == NULL)
        {
            /* IPパケットはポートでしかフィルタリングできない */
            return FALSE;
        }

        switch (ip->proto)
        {
            //case IPv4_PROTO_TCP:
            /* TODO */
            //      break;
            //case IPv4_PROTO_UDP:
            /* TODO */
            //      break;
            //case IPv4_PROTO_ICMP:
            /* TODO */
            //      break;
        default:
            return FALSE;
        }
    }

    return TRUE;
}
