/**
 * @file snoopPrintEthernet.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <ethernet.h>
#include <network.h>
#include <snoop.h>
#include <stdio.h>

/**
 * @ingroup snoop
 *
 * Ethernetパケットの内容を出力する
 * @param ether Ethernetパケット
 * @param verbose 詳細レベル
 * @return 出力に成功した場合は ::OK; エラーが発生した場合は ::SYSERR
 */
int snoopPrintEthernet(struct etherPkt *ether, char verbose)
{
    char output[40];
    struct netaddr hwaddr;

    /* Error check pointer */
    if (NULL == ether)
    {
        return SYSERR;
    }

    if (verbose >= SNOOP_VERBOSE_TWO)
    {
        /* Print ethernet header */
        printf(" ----- Ethernet Header -----\n");
        hwaddr.type = NETADDR_ETHERNET;
        hwaddr.len = ETH_ADDR_LEN;
        memcpy(hwaddr.addr, ether->dst, hwaddr.len);
        netaddrsprintf(output, &hwaddr);
        printf("  Dst: %-25s ", output);
        memcpy(hwaddr.addr, ether->src, hwaddr.len);
        netaddrsprintf(output, &hwaddr);
        printf("Src: %-25s ", output);
        switch (net2hs(ether->type))
        {
        case ETHER_TYPE_IPv4:
            sprintf(output, "IPv4");
            break;
        case ETHER_TYPE_ARP:
            sprintf(output, "ARP");
            break;
        case ETHER_TYPE_IPv6:
            sprintf(output, "IPv6");
            break;
        default:
            sprintf(output, "0x%04X", net2hs(ether->type));
            break;
        }
        printf("Type: %-5s\n", output);
    }

    return OK;
}
