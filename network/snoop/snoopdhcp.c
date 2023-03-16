/**
 * @file snoophdcp.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <stdint.h>
#include <network.h>
#include <stdio.h>
#include <string.h>
#include <snoop.h>
#include <dhcp.h>

static void snoopPrintDhcpOperation(uchar op, char *descrp)
{
    switch(op)
    {
    case DHCP_OP_REQUEST:
        strcpy(descrp, "(REQUEST)");
        break;
    case DHCP_OP_REPLY:
        strcpy(descrp, "(REPLY)");
        break;
    default:
        strcpy(descrp, "");
        break;
    }
}

/**
 * @ingroup snoop
 * DHCPパケットの内容を出力する.
 * @param dhcp DHCPパケット
 * @param verbose 詳細レベル
 * @return 出力に成功した場合は ::OK; エラーが発生した場合は ::SYSERR
 */
int snoopPrintDhcp(struct dhcpPkt *dhcp, char verbose)
{
    char descrp[40];
    char output[40];
    struct netaddr praddr;
    uint32_t val = 0;

    int done = 0;
    uchar *opts = NULL;
    ushort len = 0;

    if (NULL == dhcp) {
        return SYSERR;
    }

    printf(" ----- DHCP Header -----\n");

    snoopPrintDhcpOperation(dhcp->op, descrp);
    sprintf(output, "%d %s", dhcp->op, descrp);
    printf("  Operation: %-25s ", output);

    if (dhcp->htype == DHCP_HW_ETHERNET) {
        strcpy(descrp, "(Ethernet)");
    } else {
        strcpy(descrp, "");
    }
    sprintf(output, "%d %s", dhcp->htype, descrp);
    printf("  HW type: %-25s\n", output);

    sprintf(output, "0x%02x (%u) bytes", dhcp->hlen, dhcp->hlen);
    printf("  HW length: %-25s ", output);
    sprintf(output, "0x%02x (%u)", dhcp->hops, dhcp->hops);
    printf("  Hops: %-25s ", output);
    sprintf(output, "0x%04x (%u)", dhcp->xid, net2hl(dhcp->xid));
    printf("  Transfer id: %-25s\n", output);
    sprintf(output, "0x%02x (%u)", dhcp->secs, net2hl(dhcp->secs));
    printf("  Seconds: %-25s ", output);

    if (net2hs(dhcp->flags) == DHCP_FLAGS_BROADCAST) {
        strcpy(descrp, "(Ethernet)");
    } else {
        strcpy(descrp, "");
    }
    sprintf(output, "0x%04x %s", net2hs(dhcp->flags), descrp);
    printf("  Falgs: %-25s\n", output);

    int2ipv4(net2hl(dhcp->ciaddr), &praddr);
    netaddrsprintf(output, &praddr);
    printf("  Client IP Addr: %-25s ", output);

    int2ipv4(net2hl(dhcp->yiaddr), &praddr);
    netaddrsprintf(output, &praddr);
    printf("  Your IP Addr: %-25s\n", output);

    int2ipv4(net2hl(dhcp->siaddr), &praddr);
    netaddrsprintf(output, &praddr);
    printf("  Server IP Addr: %-25s ", output);

    int2ipv4(net2hl(dhcp->giaddr), &praddr);
    netaddrsprintf(output, &praddr);
    printf("  Gateway IP Addr: %-25s\n", output);

    printf("  Client Hardware Addr: %02x:%02x:%02x:%02x:%02x:%02x ",
        dhcp->chaddr[0], dhcp->chaddr[1], dhcp->chaddr[2],
        dhcp->chaddr[3], dhcp->chaddr[4], dhcp->chaddr[5]);
    printf("  Server name: %s\n", dhcp->sname);
    printf("  File: %s\n", dhcp->file);

    opts = dhcp->opts;
    while(!done)
    {
        switch(*opts)
        {
        case DHCP_OPT_END:
            printf( "  End Option (255)\n");
            done = TRUE;
            break;

        case DHCP_OPT_PAD:
            break;

        case DHCP_OPT_SUBNET:
            opts++;
            len = *opts++;
            if (len != 4)  break;
            printf("  Subnet Mask Option (1): ");
            for (int i = 0; i < len; i++) {
                if (i > 0) printf(".");
                printf("%d", *opts++);
            }
            opts--;
            printf( "\n");
            break;

        case DHCP_OPT_GATEWAY:
            opts++;
            len = *opts++;
            if (0 != (len % 4) || len == 0) break;
            printf("  Router Option (3):\n");
            for (int i = 0; i < len / 4; i++) {
                printf("    gateway addr[%d]: ", i);
                for (int j = 0; j < 4; j++) {
                    if (j > 0) printf(".");
                    printf("%d", *opts++);
                }
                printf( "\n");
            }
            opts--;
            break;

        case DHCP_OPT_DNS:
            opts++;
            len = *opts;
            if (0 != (len % 4) || 0 == len)  break;
            printf("  Domain Name Server Option (6):\n");
            for (int i = 0; i < len / 4; i++) {
                printf("    DNS addr[%d]: ", i);
                for (int j = 0; j < 4; j++) {
                    if (j > 0) printf(".");
                    printf("%d", *opts++);
                }
                printf( "\n");
            }
            opts--;
            break;

        case DHCP_OPT_HNAME:
            opts++;
            len = *opts++;
            if (0 == len) break;
            printf("  Host Name Option (12): ");
            for (int i = 0; i < len; i++) {
                if (i > 0) printf(".");
                printf("%d", *opts++);
            }
            opts--;
            printf("\n");
            break;
        case DHCP_OPT_DOMAIN:
            opts++;
            len = *opts++;
            if (0 == len) break;
            printf("  Domain Name Option (15): ");
            for (int i = 0; i < len; i++) {
                printf("%c", *opts++);
            }
            printf( "\n");
            break;

        case DHCP_OPT_REQUEST:
            opts++;
            len = *opts++;
            if (len != 4) break;
            printf("  Requested IP Address Option (50): ");
            for (int i = 0; i < len; i++) {
                if (i > 0) printf(".");
                printf("%d", *opts++);
            }
            opts--;
            printf("\n");
            break;

        case DHCP_OPT_LEASE:
            opts++;
            len = *opts++;
            if (len != 4) break;
            val = ((uint32_t)(*opts++) << 24) | ((uint32_t)(*opts++) << 16) |
                  ((uint32_t)(*opts++) << 8)  | ((uint32_t)(*opts++) << 0);
            printf("  Address Lease Time Option (51): ");
            printf("%u\n", val);
            break;

        case DHCP_OPT_MSGTYPE:
            opts++;
            len = *opts++;
            if (len != 1) break;
            printf("Message Type Option (53): ");
            switch(*opts)
            {
            case DHCPDISCOVER:
                printf( "DHCPDISCOVER\n");
                break;
            case DHCPOFFER:
                printf( "DHCPOFFER\n");
                break;
            case DHCPREQUEST:
                printf( "DHCPREQUEST\n");
                break;
            case DHCPDECLINE:
                printf( "DHCPDECLINE\n");
                break;
            case DHCPACK:
                printf( "DHCPACK\n");
                break;
            case DHCPNAK:
                printf( "DHCPNAK\n");
                break;
            case DHCPRELEASE:
                printf( "DHCPRELEASE\n");
                break;
            default:
                printf( "Unrecognized\n");
                break;
            }
            break;

        case DHCP_OPT_SERVER:
            opts++;
            len = *opts++;
            if (len != 4) break;
            printf("  Server Identifier Option (54): ");
            for (int i = 0; i < len; i++) {
                if (i > 0) printf(".");
                printf("%d", *opts++);
            }
            opts--;
            printf("\n");
            break;

        case DHCP_OPT_PARAMREQ:
            opts++;
            len = *opts++;
            if (0 == len) break;
            printf("  Parameter Request List Option (55):\n");
            for (int i = 0; i < len; i++)
            {
                switch(*opts++)
                {
                case DHCP_OPT_SUBNET:
                    printf("      Subnet Option\n");
                    break;
                case DHCP_OPT_GATEWAY:
                    printf("      Gateway Option\n");
                    break;
                case DHCP_OPT_DNS:
                    printf("      DNS Option\n");
                    break;
                case DHCP_OPT_HNAME:
                    printf("      Host Name Option\n");
                    break;
                case DHCP_OPT_DOMAIN:
                    printf("      Domain Name Option\n");
                    break;
                case DHCP_OPT_REQUEST:
                    printf("      Request Option\n");
                    break;
                case DHCP_OPT_LEASE:
                    printf("      Lease Option\n");
                    break;
                case DHCP_OPT_SERVER:
                    printf("      Server Option\n");
                    break;
                default:
                    printf("      Unrecognized Option %d\n", *opts);
                    break;
                }
            }
            break;

        default:
            opts++;
            len = *opts;

            opts += len;
            break;
        }
    }

    return OK;
}
