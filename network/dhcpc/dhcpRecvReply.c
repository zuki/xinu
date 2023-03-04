/**
 * @file dhcpRecvReply.c
 *
 */
/* Embedded Xinu, Copyright (C) 2008, 2013.  All rights reserved. */

#include <device.h>
#include "dhcp.h"
#include <ether.h>
#include <ipv4.h>
#include <network.h>
#include <string.h>
#include <stdlib.h>
#include <udp.h>
#include <thread.h>
#include <core.h>

#define DHCP_RECV_STKSIZE NET_THR_STK
#define DHCP_RECV_PRIO    NET_THR_PRIO

/* Stress testing--- randomly ignore this percent of valid received data
 * packets.  */
#define DHCP_DROP_PACKET_PERCENT 0

static thread do_dhcpRecvReply(int descrp, struct dhcpData *data,
                               struct packet *pkt);

/**
 * @ingroup dhcpc
 *
 * DHCPサーバからの応答をタイムアウト付きで待機し、DHCP転送データを更新する.
 * この関数の正確な動作は現在のDHCPクライアントの状態に依存する。
 *
 * - DHCPC_STATE_SELECTINGの場合, クライアントは任意のサーバからの
 *   DHCPOFFER応答を取得するまで待機する.
 * - DHCPC_STATE_REQUESTINGの場合, クライアントはDHCPREQUESTを送信した
 *   サーバからのDHCPACK応答、またはDHCPNAK応答を取得するまで待機する.
 *
 * @param[in] descrp
 *      応答を退位するネットワークデバイス
 * @param[in,out] data
 *      DHCP転送データ
 * @param[in] timeout
 *      ミリ秒単位の待機タイムアウト時間
 *
 * @return
 *      成功の場合は OK; タイムアウトの場合は TIMEOUT; エラーが発生した場合は SYSERR
 */
syscall dhcpRecvReply(int descrp, struct dhcpData *data, uint timeout)
{
    struct packet *pkt;
    tid_typ tid;
    int retval;

    pkt = netGetbuf();
    if (SYSERR == (int)pkt)
    {
        return SYSERR;
    }

    /* これはタイムアウトを実装するためのちょっとしたハックである:  read()でブロック
     * するのを避けるために別のスレッドで応答を待機する  */
    tid = create(do_dhcpRecvReply, DHCP_RECV_STKSIZE, DHCP_RECV_PRIO,
                 "dhcpRecvReply", 3, descrp, data, pkt);
    if (isbadtid(tid))
    {
        netFreebuf(pkt);
        return SYSERR;
    }

    ready(tid, RESCHED_YES, CORE_ZERO);

    /* TIMEOUTを返す前にスレッドが終了するまで最大 @timeout ミリ秒待機する */
    if (TIMEOUT == recvtime(timeout))
    {
        kill(tid);
        receive();
        retval = TIMEOUT;
    }
    else
    {
        retval = data->recvStatus;
    }
    netFreebuf(pkt);
    return retval;
}

/**
 * dhcpRecvReply()のコメントを適用するが、do_dhcpRecvReply()ではタイムアウトはなく、
 * 別のスレッドで実行される。さらに、このスレッドはいつでも終了させることができるので、
 * 独自のメモリを確保するのではなく、@p pkt パラメータで渡されたメモリを使用しなければ
 * ならない。
 */
static thread do_dhcpRecvReply(int descrp, struct dhcpData *data,
                               struct packet *pkt)
{
    const struct etherPkt *epkt;
    const struct ipv4Pkt *ipv4;
    const struct udpPkt *udp;
    const struct dhcpPkt *dhcp;
    const uchar *opts;
    uint maxlen;
    int found_msg;
    int retval;
    const uchar *gatewayptr;
    const uchar *maskptr;
    const uchar *opts_end;
    uint serverIpv4Addr;
    int mtu;
    int linkhdrlen;

    mtu = control(descrp, NET_GET_MTU, 0, 0);
    linkhdrlen = control(descrp, NET_GET_LINKHDRLEN, 0, 0);
    if (SYSERR == mtu || SYSERR == linkhdrlen)
    {
        return SYSERR;
    }

    maxlen = linkhdrlen + mtu;

    /* 待機している応答を見つけるまでパケットを受信する  */

next_packet:
    do
    {
        /* 1. ネットワークデバイスから次のパケットを受信する  */
        int len = read(descrp, pkt->data, maxlen);
        if (len == SYSERR || len <= 0)
        {
            data->recvStatus = SYSERR;
            return SYSERR;
        }

        pkt->len = len;
        DHCP_TRACE("Received packet (len=%u).", pkt->len);

        /* 2. パケットが少なくとも最小DHCPパケット長はあるかチェックする */
        if (pkt->len < (ETH_HDR_LEN + IPv4_HDR_LEN +
                        UDP_HDR_LEN + DHCP_HDR_LEN))
        {
            DHCP_TRACE("Too short to be a DHCP packet.");
            goto next_packet;
        }

        /* 3. ヘッダーポインタを設定する  */
        epkt = (const struct etherPkt *)pkt->data;
        ipv4 = (const struct ipv4Pkt *)epkt->data;
        udp = (const struct udpPkt *)ipv4->opts;
        dhcp = (const struct dhcpPkt *)udp->data;

        /* 4. パケットの妥当性をチェックする。
         * DHCPパケットは、種別はIPv4, プロトコルはUDP, UDP宛先ポートはDHCPC,
         * UDP送信元ポートはDHCPSでなければならない。
         *
         * また、DHCPヘッダーも、実際にそれがこのクライアントへの応答であるかチェックする
         * */
        if ((ETHER_TYPE_IPv4 != net2hs(epkt->type))
            || (IPv4_PROTO_UDP != ipv4->proto)
            || (UDP_PORT_DHCPC != net2hs(udp->dstPort))
            || (UDP_PORT_DHCPS != net2hs(udp->srcPort))
            || (DHCP_OP_REPLY != dhcp->op)
            || (data->cxid != net2hl(dhcp->xid)))
        {
            DHCP_TRACE("Not a DHCP reply to this client.");
            goto next_packet;
        }

        DHCP_TRACE("Received DHCP reply.");

    #if DHCP_DROP_PACKET_PERCENT != 0
        /* Stress testing.  */
        if (rand() % 100 < DHCP_DROP_PACKET_PERCENT)
        {
            DHCP_TRACE("WARNING: Ignoring valid DHCP packet for test purposes.");
            goto next_packet;
        }
    #endif

        /* 5. DHCPオプションをパースする
         * 正しいDHCP応答を取得したのでDHCPオプションをパースする。オプションデータが
         * 無効な場合にパケットバッファをオーバーランしないように慎重に行う必要がある。
         */
        opts = dhcp->opts;
        maskptr = NULL;
        gatewayptr = NULL;
        serverIpv4Addr = 0;
        opts_end = opts + (pkt->len - (ETH_HDR_LEN + IPv4_HDR_LEN +
                                       UDP_HDR_LEN + DHCP_HDR_LEN));
        found_msg = -1;
        for (;;)
        {
            uchar opt, len;

            /* a. 次のオプション種別を取得する */
            if (opts >= opts_end)
            {
                DHCP_TRACE("Invalid DHCP options.");
                goto next_packet;
            }
            opt = *opts++;

            /* DHCP_OPT_ENDの場合はbreak （DHCPオプションの終了マークを付ける） */
            if (DHCP_OPT_END == opt)
            {
                DHCP_TRACE("Reached DHCP_OPT_END.");
                break;
            }

            /* このオプションのデータ長を取得する */
            if (opts >= opts_end)
            {
                DHCP_TRACE("Invalid DHCP options.");
                goto next_packet;
            }
            len = *opts++;

            if (opts + len >= opts_end)
            {
                DHCP_TRACE("Invalid DHCP options.");
                goto next_packet;
            }

            /* 指定されたDHCPオプションを処理する。未知のオプションは無視する */
            switch (opt)
            {
            case DHCP_OPT_MSGTYPE:
                DHCP_TRACE("DHCP_OPT_MSGTYPE: %d", *opts);
                if (len >= 1)
                {
                    if ((DHCPC_STATE_SELECTING == data->state &&
                         *opts == DHCPOFFER)
                        || (DHCPC_STATE_REQUESTING == data->state &&
                            (DHCPACK == *opts || DHCPNAK == *opts)))
                    {
                        found_msg = *opts;
                    }
                }
                break;

            case DHCP_OPT_SUBNET:
                if (len >= IPv4_ADDR_LEN)
                {
                    maskptr = opts;
                }
                break;

            case DHCP_OPT_GATEWAY:
                if (len >= IPv4_ADDR_LEN)
                {
                    gatewayptr = opts;
                }
                break;

            case DHCP_OPT_SERVER:
                if (len >= IPv4_ADDR_LEN)
                {
                    /* Server Identifier option.  */
                    serverIpv4Addr = ((uint)opts[0] << 24) |
                                     ((uint)opts[1] << 16) |
                                     ((uint)opts[2] << 8) |
                                     ((uint)opts[3] << 0);
                }
                break;
            }

            /* オプションのデータ帳だけすすめる */
            opts += len;
        }
    } while (found_msg < 0);

    /* We received a reply of at least the right type as one we were looking
     * for.  */

    /* サブネットマスクが提供されない限りDHCPACK応答を妥当だとはみなさない  */
    if (DHCPACK == found_msg && NULL == maskptr)
    {
        DHCP_TRACE("Ignoring DHCPACK (no subnet mask provided).");
        goto next_packet;
    }

    /* 注意: サーバーのIPアドレスはServer Identifierオプションで指定することに
     * なっており、siaddr（Server IP Address）フィールドでは*ない*。これは
     * やや直感的ではないが、siaddrはブートストラッププロセスの次のサーバの
     * アドレスに使用され、これはDHCPサーバと同じでない場合があるからである。
     * ただし、Server Identifierオプションが存在しなかった場合はsiaddrを使用する。
     */
    if (0 == serverIpv4Addr)
    {
        serverIpv4Addr = net2hl(dhcp->siaddr);
        if (0 == serverIpv4Addr)
        {
            DHCP_TRACE("Server IP address empty.");
            goto next_packet;
        }
    }

    if (DHCPOFFER == found_msg)
    {
        /* DHCPOFFER:  オファーされたアドレスとサーバアドレスを記録する */
        data->serverIpv4Addr = serverIpv4Addr;
        data->offeredIpv4Addr = net2hl(dhcp->yiaddr);
        memcpy(data->serverHwAddr, epkt->src, ETH_ADDR_LEN);
        retval = OK;
    }
    else
    {
        /* DHCPACK または DHCPNAKを受信した。それがDHCPREQUESTを送信したサーバと
         * 同じであるか確認して、そうでなければパケットを無視する。 */
        if (serverIpv4Addr != data->serverIpv4Addr)
        {
            DHCP_TRACE("Reply from wrong server.");
            goto next_packet;
        }

        if (DHCPNAK == found_msg)
        {
            /* DHCPNAK: クライアントが再度DHCPDISCOVERを試行できるようにエラーを返す */
            retval = SYSERR;
        }
        else
        {
            /* DHCPACK:  ネットワークインタフェースアドレスをセットする */
            data->ip.type = NETADDR_IPv4;
            data->ip.len = IPv4_ADDR_LEN;
            /* DHCPACKのyiaddrはDHCPOFFERから格納したyiaddrと同じはずであるが、
             * DHCPACKの値を使うのが望ましい。それがサーバが付与したと考える
             * 値だからである。 */
            memcpy(data->ip.addr, &dhcp->yiaddr, IPv4_ADDR_LEN);
            data->clientIpv4Addr = net2hl(dhcp->yiaddr);

            data->mask.type = NETADDR_IPv4;
            data->mask.len = IPv4_ADDR_LEN;
            memcpy(data->mask.addr, maskptr, IPv4_ADDR_LEN);

            if (NULL != gatewayptr)
            {
                data->gateway.type = NETADDR_IPv4;
                data->gateway.len = IPv4_ADDR_LEN;
                memcpy(data->gateway.addr, gatewayptr, IPv4_ADDR_LEN);
            }

            /* DHCPACKで提供されていたら次のサーバのアドレスとbootファイル
             * （たとえばTFTP用）をセットする */
            if (0 != dhcp->siaddr)
            {
                data->next_server.type = NETADDR_IPv4;
                data->next_server.len = IPv4_ADDR_LEN;
                memcpy(data->next_server.addr, &dhcp->siaddr, IPv4_ADDR_LEN);
            }
            if ('\0' != dhcp->file[0])
            {
                memcpy(data->bootfile, dhcp->file, sizeof(data->bootfile) - 1);
            }
        #ifdef ENABLE_DHCP_TRACE
            {
                char str_addr[24];
                netaddrsprintf(str_addr, &data->ip);
                DHCP_TRACE("Set ip=%s", str_addr);

                netaddrsprintf(str_addr, &data->mask);
                DHCP_TRACE("Set mask=%s", str_addr);

                if (NULL != gatewayptr)
                {
                    netaddrsprintf(str_addr, &data->gateway);
                    DHCP_TRACE("Set gateway=%s", str_addr);
                }
                else
                {
                    DHCP_TRACE("No gateway.");
                }

                netaddrsprintf(str_addr, &data->next_server);
                DHCP_TRACE("TFTP server=%s", str_addr);
                DHCP_TRACE("Bootfile=%s", data->bootfile);
            }
        #endif
            retval = OK;
        }
    }
    data->recvStatus = retval;
    return retval;
}
