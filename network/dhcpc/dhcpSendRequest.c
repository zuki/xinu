/**
 * @file     dhcpSendRequest.c
 *
 */
/* Embedded Xinu, Copyright (C) 2008, 2013.  All rights reserved. */

#include <clock.h>
#include <device.h>
#include "dhcp.h"
#include <ethernet.h>
#include <ipv4.h>
#include <network.h>
#include <string.h>
#include <udp.h>

/**
 * @ingroup dhcpc
 * DHCPクライアントの現在の状態に基づいて、DHCPDISCOVER (DHCPC_STATE_INITの
 * 場合）または DHCPREQUEST (DHCPC_STATE_SELECTINGの場合） パケットを送信する.
 *
 * @param[in] descrp
 *      DHCPパケットを送信するネットワークデバイス
 * @param[in,out] data
 *      DHCPC_STATE_INIT または DHCPC_STATE_SELECTING の状態に合わせて
 *      セットされたDHCP転送データ
 *
 * @return
 *      パケットの送信に成功したら OK; そうでなければ SYSERR
 */
syscall dhcpSendRequest(int descrp, struct dhcpData *data)
{
    struct packet *pkt;
    struct etherPkt *ether;
    struct ipv4Pkt *ipv4;
    struct udpPkt *udp;
    struct dhcpPkt *dhcp;
    int retval;
    uint pktsize;
    uchar optarray[128];
    uchar *opts;
    uint optsize;
    uint tmp_ipv4addr;

    /* 1. DHCPオプションを構築 */
    opts = optarray;
    *opts++ = DHCP_OPT_MSGTYPE;     // Opt 53: メッセージ種別
    *opts++ = 1;                    // 長さ: 1
    switch (data->state)
    {
        case DHCPC_STATE_INIT:
            *opts++ = DHCPDISCOVER;         // 1: メッセージ種別: Discover

            *opts++ = DHCP_OPT_PARAMREQ;    // Opt 55: パラメタリクエストリスト
            *opts++ = 2;                    // 長さ: 2
            *opts++ = DHCP_OPT_SUBNET;      // 1: サブネットマスクの払い出し
            *opts++ = DHCP_OPT_GATEWAY;     // 3: ゲートウェイIPアドレス
            break;

        case DHCPC_STATE_SELECTING:
            *opts++ = DHCPREQUEST;          // 3: メッセージ種別: リクエスト

            *opts++ = DHCP_OPT_REQUEST;     // Opt 50: リクエスト
            *opts++ = IPv4_ADDR_LEN;        // 4: IPv4アドレス長
            tmp_ipv4addr = hl2net(data->offeredIpv4Addr);
            memcpy(opts, &tmp_ipv4addr, IPv4_ADDR_LEN);
            opts += IPv4_ADDR_LEN;

            *opts++ = DHCP_OPT_SERVER;      // Opt 54: サーバ
            *opts++ = IPv4_ADDR_LEN;
            tmp_ipv4addr = hl2net(data->serverIpv4Addr);
            memcpy(opts, &tmp_ipv4addr, IPv4_ADDR_LEN);
            opts += IPv4_ADDR_LEN;

            /* RFC 2131: 「クライアントはDHCPDISCOVERメッセージに要求
             * パラメータリストを含めた場合、その後のすべてのメッセージに
             * そのリストを含めなければならない（MUST）」
             */
            *opts++ = DHCP_OPT_PARAMREQ;
            *opts++ = 2;
            *opts++ = DHCP_OPT_SUBNET;
            *opts++ = DHCP_OPT_GATEWAY;
            break;

        default:
            return SYSERR;
    }

    /* オプションを終了させるには少なくとも1つleast one DHCP_OPT_ENDバイトを
     * 追加するが、オプションのサイズが4バイトの倍数に成るよう必要に応じて
     * 更に追加する。  */
    do
    {
        *opts++ = DHCP_OPT_END;
    } while ((optsize = (opts - optarray)) & 3);

    /* 2. パケット用のバッファを取得する */
    pkt = netGetbuf();
    if (SYSERR == (int)pkt)
    {
        return SYSERR;
    }

    /* 3. 他のレイアのパケットヘッダーへのポインタを設定する */
    pktsize = ETH_HDR_LEN + IPv4_HDR_LEN + UDP_HDR_LEN + DHCP_HDR_LEN + optsize;
    pkt->len += pktsize;
    pkt->curr -= pktsize;

    ether = (struct etherPkt*)pkt->curr;
    ipv4 = (struct ipv4Pkt *)ether->data;
    udp = (struct udpPkt *)ipv4->opts;
    dhcp = (struct dhcpPkt *)udp->data;

    /* 4. DHCPパケットを構築する */
    dhcp->op = DHCP_OP_REQUEST;
    dhcp->htype = DHCP_HTYPE_ETHER;
    dhcp->hlen = ETH_ADDR_LEN;
    dhcp->hops = 0;
    dhcp->xid = hl2net(data->cxid);
    if (data->state == DHCPC_STATE_SELECTING)
    {
        /* RFC 2131: 「... DHCPREQUESTメッセージはDHCPメッセージの
         * "secs"フィールドに ... 元のDHCPDISCOVERメッセージと同じ値を
         * 使用しなければならない」 */
        dhcp->secs = hs2net(data->discoverSecs);
    }
    else
    {
        dhcp->secs = hs2net(clktime - data->starttime);
        if (data->state == DHCPC_STATE_INIT)
        {
            data->discoverSecs = net2hs(dhcp->secs);
        }
    }

    dhcp->flags = 0x0000;
    dhcp->ciaddr = hl2net(data->clientIpv4Addr);
    dhcp->yiaddr = 0; /* RFC 2131:  client must set yiaddr to 0.  */
    dhcp->siaddr = 0; /* RFC 2131:  client must set siaddr to 0.  */
    dhcp->giaddr = 0; /* RFC 2131:  client must set giaddr to 0.  */
    memcpy(dhcp->chaddr, data->clientHwAddr, dhcp->hlen);
    dhcp->cookie = hl2net(DHCP_MAGICCOOKIE);
    memcpy(dhcp->opts, optarray, optsize);

    /* 5. UDPパケットを構築する */
    udp->srcPort = hs2net(UDP_PORT_DHCPC);
    udp->dstPort = hs2net(UDP_PORT_DHCPS);
    udp->len = hs2net(UDP_HDR_LEN + DHCP_HDR_LEN + optsize);

    /* 6. UDP疑似ヘッダパケットのIPv4部分を設定する */
    ipv4->proto = IPv4_PROTO_UDP;
    memset(ipv4->src, 0, IPv4_ADDR_LEN);
    memset(ipv4->dst, 0xff, IPv4_ADDR_LEN);
    ipv4->ttl = 0;
    ipv4->chksum = udp->len;
    udp->chksum = netChksum((uchar *)&ipv4->ttl,
                            (sizeof(struct udpPseudoHdr) + UDP_HDR_LEN +
                             DHCP_HDR_LEN + optsize));

    /* 7. IPv4パケットを設定する */
    ipv4->ver_ihl = (IPv4_VERSION << 4) | (IPv4_HDR_LEN / 4);
    ipv4->tos = IPv4_TOS_ROUTINE;
    ipv4->len = hs2net(pktsize - ETH_HDR_LEN);
    ipv4->id = 0;
    ipv4->flags_froff = hs2net(IPv4_FLAG_DF | 0x0000);
    ipv4->ttl = IPv4_TTL;
    ipv4->chksum = 0;
    ipv4->chksum = netChksum((uchar *)ipv4, IPv4_HDR_LEN);

    /* 8. Ethernetパケットを設定して、ハードウェアブロードキャスト
     *    アドレスに送信する
     */
    memset(ether->dst, 0xff, ETH_ADDR_LEN);
    memcpy(ether->src, data->clientHwAddr, ETH_ADDR_LEN);
    ether->type = hs2net(ETHER_TYPE_IPv4);
    if (pktsize == write(descrp, pkt->curr, pktsize))
    {
        retval = OK;
    }
    else
    {
        retval = SYSERR;
    }
    netFreebuf(pkt);
    return retval;
}
