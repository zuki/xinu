/**
 * @file snoophdcp.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <network.h>
#include <snoop.h>
#include <stdio.h>
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

    int done = 0;
    uchar *opts = NULL;
    ushort len = 0;
    ulong i = 0;

    if (NULL == dhcp) {
        return SYSERR;
    }

    printf(stdout," ----- DHCP Header -----\n");

    snoopPrintDhcpOperation(dhcp->op, descrp);
    sprintf(output, "%d %s", dhcp->op, descrp);
    printf("  Operation: %-25s ", output);

    if (dhcp->htype == DHCP_HW_ETHERNET) {
        strcpy(descrp, "(Ethernet)");
    } else {
        strcpy(descrp, "");
    }
    sprintf(output, "%d %s", dhcp->htype, descrp);
    printf("  HW type: %-25s ", output);

    sprintf(output, "%d bytes", dhcp->hlen);
    printf("  HW length: %-20s ", output);
    sprintf(output, "%d", dhcp->hops);
    printf("  Hops: %-20s\n", output);
    printf("  Transfer id: %d\n", net2hl(dhcp->xid));
    printf("  Seconds = %d\n", net2hs(dhcp->secs));

    if (net2hs(dhcp->flags) == DHCP_FLAGS_BROADCAST) {
        strcpy(descrp, "(Ethernet)");
    } else {
        strcpy(descrp, "");
    }
    sprintf(output, "0x%04x %s", net2hs(dhcp->flags), descrp);
    printf("  Falgs: %-25s\n", output);

    praddr.type = NETADDR_IPv4;
    praddr.len = IPv4_ADDR_LEN;
    memcpy(praddr.addr, net2hl(dhcp->ciaddr), praddr.len);
    netaddrsprintf(output, &praddr);
    printf("  Client IP Addr: %-25s ", output);

    memcpy(praddr.addr, net2hl(dhcp->yiaddr), praddr.len);
    netaddrsprintf(output, &praddr);
    printf("  Your IP Addr: %-25s\n", output);

    memcpy(praddr.addr, net2hl(dhcp->siaddr), praddr.len);
    netaddrsprintf(output, &praddr);
    printf("  Server IP Addr: %-25s\n", output);

    memcpy(praddr.addr, net2hl(dhcp->giaddr), praddr.len);
    netaddrsprintf(output, &praddr);
    printf("  Gateway IP Addr: %-25s\n", output);

    printf("  Client Hardware Addr: %02x:%02x:%02x:%02x:%02x:%02x ",
        dhcp->chaddr[0], dhcp->chaddr[1], dhcp->chaddr[2],
        dhcp->chaddr[3], dhcp->chaddr[4], dhcp->chaddr[5]);
    printf("  Server name: %-64s\n", dhcp->sname);
        printf("  File: %-128s\n", dhcp->file);

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
            len = *opts;
            if(len != 4)
            { break; }
            printf("  Subnet Mask Option (1)\n");
            printf("    length = %d\n", len);
            opts++;
            printf("    subnet addr = %d", *opts);
            len--;
            while (0 != len)
            {
                opts++;
                printf(".%d", *opts);
                len--;
            }
            printf( "\n");
            break;

        case DHCP_OPT_GATEWAY:
            opts++;
            len = *opts;
            if(0 != (len % 4) || len == 0)
            { break; }
            printf("  Router Option (3)\n");
            printf("    length = %d\n", len);
            opts++;
            i = 1;
            printf("    gateway addr %d = %d", i, *opts);
            len--;
            while (0 != len)
            {
                opts++;
                if(0 == (len % 4))
                {
                    i++;
                    printf( "\n\t\tgateway addr %d = %d", i, *opts);
                    opts++;
                    len--;
                }
                printf(".%d", *opts);
                len--;
            }
            printf( "\n");
            break;

        case DHCP_OPT_DNS:
            opts++;
            len = *opts;
            if (0 != (len % 4) || 0 == len)
            { break; }
            printf("  Domain Name Server Option (6)\n");
            printf("    length = %d\n", len);
            opts++;
            i = 1;
            printf("    DNS addr %d = %d", i, *opts);
            len--;
            while (0 != len)
            {
                opts++;
                if(0 == (len % 4))
                {
                    i++;
                    printf("\n    DNS addr %d = %d", i, *opts);
                    len--;
                    opts++;
                }
                printf(".%d", *opts);
                len--;
            }
            printf( "\n");
            break;

        case DHCP_OPT_HNAME:
            opts++;
            len = *opts;
            if (0 == len)
            { break; }
            printf("  Host Name Option (12)\n");
            printf("    length = %d\n", len);
            opts++;
            printf("    host name = %c", *opts);
            len--;
            while (0 != len)
            {
                opts++;
                printf("%c", *opts);
                len--;
            }
            printf( "\n");

        case DHCP_OPT_DOMAIN:
            opts++;
            len = *opts;
            if(0 == len)
            { break; }
            printf("  Domain Name Option (15)\n");
            printf("    length = %d\n", len);
            opts++;
            printf("    domain name = %c", *opts);
            len--;
            while (0 != len)
            {
                opts++;
                printf("%c", *opts);
                len--;
            }
            printf( "\n");
            break;

        case DHCP_OPT_REQUEST:
            opts++;
            len = *opts;
            if(len != 4)
            { break; }
            printf("  Requested IP Address Option (50)\n");
            printf("    length = %d\n", len);
            opts++;
            printf("    requested addr = %d", *opts);
            len--;
            while (0 != len)
            {
                opts++;
                printf(".%d", *opts);
                len--;
            }
            printf( "\n");
            break;

        case DHCP_OPT_LEASE:
            opts++;
            len = *opts;
            if(len != 4)
            { break; }
            printf("  Address Lease Time Option (51)\n");
            printf("    length = %d\n", len);
            opts++;
            i+= *opts<<24;
            opts++;
            i+= *opts<<16;
            opts++;
            i+= *opts<<8;
            opts++;
            i+= *opts;
            printf("    lease time = %u\n", i);
            break;

        case DHCP_OPT_MSGTYPE:
            opts++;
            len = *opts;
            if(len != 1)
            { break; }
            printf("Message Type Option (53)\n");
            printf("    length = %d\n", len);
            printf("    type = ");
            opts++;
            switch(*opts)
            {
            case DHCPDISCOVER:
                printf( "DHCPDISCOVER");
                break;
            case DHCPOFFER:
                printf( "DHCPOFFER");
                break;
            case DHCPREQUEST:
                printf( "DHCPREQUEST");
                break;
            case DHCPDECLINE:
                printf( "DHCPDECLINE");
                break;
            case DHCPACK:
                printf( "DHCPACK");
                break;
            case DHCPNAK:
                printf( "DHCPNAK");
                break;
            case DHCPRELEASE:
                printf( "DHCPRELEASE");
                break;
            default:
                printf( "Unrecognized");
                break;
            }
            printf( "\n");
            break;

        case DHCP_OPT_SERVER:
            opts++;
            len = *opts;
            if(len != 4)
            { break; }
            printf("  Server Identifier Option (54)\n");
            printf("    length = %d\n", len);
            opts++;
            printf("    server addr = %d", *opts);
            len--;
            while (0 != len)
            {
                opts++;
                printf(".%d", *opts);
                len--;
            }
            printf( "\n");
            break;
        case DHCP_OPT_PARAMREQ:
            opts++;
            len = *opts;
            if (0 == len)
            { break; }
            printf("  Parameter Request List Option (55)\n");
            printf("    length = %d\n", len);
            printf("    Request List:\n");
            while (0 != len)
            {
                opts++;
                switch(*opts)
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
                len--;
            }

        default:
            opts++;
            len = *opts;

            opts+=len;
            break;
        }

        opts++;
    }
}
