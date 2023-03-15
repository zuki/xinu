/**
 * @file snoopPrint.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <arp.h>
#include <ctype.h>
#include <ethernet.h>
#include <ipv4.h>
#include <network.h>
#include <snoop.h>
#include <stdio.h>
#include <string.h>
#include <tcp.h>
#include <udp.h>
#include <dhcp.h>

/**
 * @ingroup snoop
 *
 * パケットのsnoop情報を出力する.
 * @param pkt パケット
 * @param dump ダンプレベル
 * @param verbose 詳細レベル
 * @return ::OK; エラーが発生した場合は ::SYSERR
 */
int snoopPrint(struct packet *pkt, char dump, char verbose)
{
    struct etherPkt *ether = NULL;
    struct arpPkt *arp = NULL;
    struct ipv4Pkt *ip = NULL;
    struct udpPkt *udp = NULL;
    uchar *appHdr = NULL;
    struct netaddr addr;
    char strA[20];
    char strB[20];
    char strC[20];
    int i = 0;
    int j = 0;
    uchar ch;

    if (NULL == pkt)
    {
        return SYSERR;
    }

    /* TODO: タイムスタンプの出力 */

    /* パケットサマリーの出力 */
    ether = (struct etherPkt *)pkt->curr;
    switch (net2hs(ether->type))
    {
    case ETHER_TYPE_ARP:
        arp = (struct arpPkt *)ether->data;
        /* strA = あて先プロトコルアドレス */
        addr.type = net2hs(arp->prtype);
        addr.len = arp->pralen;
        memcpy(addr.addr, &arp->addrs[ARP_ADDR_DPA(arp)], addr.len);
        netaddrsprintf(strA, &addr);
        /* strB = 送信元プロトコルアドレス */
        memcpy(addr.addr, &arp->addrs[ARP_ADDR_SPA(arp)], addr.len);
        netaddrsprintf(strB, &addr);
        /* strC = 送信元ハードウェアアドレス */
        addr.type = net2hs(arp->hwtype);
        addr.len = arp->hwalen;
        memcpy(addr.addr, &arp->addrs[ARP_ADDR_SHA(arp)], addr.len);
        netaddrsprintf(strC, &addr);
        /* 操作に基づいてサマリーを出力する */
        switch (net2hs(arp->op))
        {
        case ARP_OP_RQST:
            printf("arp who-has %s tell %s", strA, strB);
            break;
        case ARP_OP_REPLY:
            printf("arp reply %s is-at %s", strA, strC);
            break;
        default:
            printf("arp unknown from %s to %s", strB, strA);
            break;
        }
        break;
    case ETHER_TYPE_IPv4:
        ip = (struct ipv4Pkt *)ether->data;
        /* strA = あて先IP */
        addr.type = NETADDR_IPv4;
        addr.len = IPv4_ADDR_LEN;
        memcpy(addr.addr, ip->dst, addr.len);
        netaddrsprintf(strA, &addr);
        /* strB = 送信元IP */
        memcpy(addr.addr, ip->src, addr.len);
        netaddrsprintf(strB, &addr);
        /* strC = プロトコル */
        strC[0] = '\0';
        switch (ip->proto)
        {
        case IPv4_PROTO_ICMP:
            sprintf(strC, "ICMP");
            break;
        case IPv4_PROTO_TCP:
            sprintf(strC, "TCP");
            break;
        case IPv4_PROTO_UDP:
            sprintf(strC, "UDP");
            break;
        }
        /* サマリーを出力する */
        printf("IP %s > %s : %s", strB, strA, strC);
        break;
    }
    printf("\n");

    /* 詳細出力 */
    if (verbose > SNOOP_VERBOSE_NONE)
    {
        snoopPrintEthernet(ether, verbose);
        switch (net2hs(ether->type))
        {
        case ETHER_TYPE_ARP:
            snoopPrintArp(arp, verbose);
            break;
        case ETHER_TYPE_IPv4:
            snoopPrintIpv4(ip, verbose);
            appHdr = (uchar *)ip + ((ip->ver_ihl & IPv4_IHL) * 4);
            switch (ip->proto)
            {
            case IPv4_PROTO_TCP:
                snoopPrintTcp((struct tcpPkt *)appHdr, verbose);
                break;
            case IPv4_PROTO_UDP:
                udp = (struct udpPkt *)appHdr;
                snoopPrintUdp(udp, verbose);
                switch(net2hs(udp->dstPort))
                {
                case UDP_PORT_DHCPS:
                case UDP_PORT_DHCPC:
                    snoopPrintDhcp(udp, verbose);
                    break;
                }
                break;
            }
            break;
        }
    }

    /* ダンプ出力 */
    if ((SNOOP_DUMP_HEX == dump) || (SNOOP_DUMP_CHAR == dump))
    {
        for (i = 0; i < pkt->len; i += 16)
        {
            printf("\t0x%04X  ", i);
            for (j = i; j < i + 16; j++)
            {
                if (0 == j % 2)
                {
                    printf(" ");
                }
                if (j >= pkt->len)
                {
                    printf("  ");
                    continue;
                }
                printf("%02X", pkt->data[j]);
            }
            if (SNOOP_DUMP_CHAR == dump)
            {
                printf("  |");
                for (j = i; (j < i + 16 && j < pkt->len); j++)
                {
                    ch = pkt->data[j];
                    if (!isascii(ch) || iscntrl(ch))
                    {
                        ch = '.';
                    }
                    printf("%c", ch);
                }
                printf("|");
            }
            printf("\n");
        }
    }

    return OK;
}
