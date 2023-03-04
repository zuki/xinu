/**
 * @file     dhcpClient.c
 */
/* Embedded Xinu, Copyright (C) 2008, 2013.  All rights reserved. */

#include <clock.h>
#include <device.h>
#include <ether.h>
#include "dhcp.h"
#include <stdlib.h>
#include <string.h>
#include <thread.h>

/**
 * @ingroup dhcpc
 *
 * IPv4自動構成用のDHCP (Dynamic Host Configuration Protocol) クライアント.
 *
 * TODO: この関数はIPアドレスが付与されたら直ちに復帰する。リース更新処理はしない。
 *
 * @param[in] descrp
 *      DHCPクライアントをオープンするネットワークデバイスのデバイスディスクリプタ.
 *      これはオープンされているが、ネットワークインターフェイスが接続されていない
 *      ネットワークデバイスである必要がある。
 * @param[in] timeout
 *      タイムアウトの秒数（正の整数でなければならない）
 * @param[out] data
 *      成功の場合、この構造体にはIPv4構成に関する情報がセットされる。少なくとも
 *      @ref dhcpData::ip "ip" と @ref dhcpData::mask "mask" には正しい値がセット
 *      される。@ref dhcpData::gateway "gateway", @ref dhcpData::bootfile "bootfile",
 *      @ref dhcpData::next_server "next_server" はオプションであり、提供されない
 *      場合はすべて0のままである。
 *
 * @return
 *      成功の場合は ::OK; DHCPDISCOVER パケットの送信に失敗した場合、
 *      または、パラメタが不正な場合は ::SYSERR; タイムアウトの場合は ::TIMEOUT
 */
syscall dhcpClient(int descrp, uint timeout, struct dhcpData *data)
{
    int retval;
    uint recvTimeout = 5000;  /* ミリ秒単位の受信のタイムアウト
                                （クライアント全体ではない） */
    uint delay = 1000;  /* ミリ秒単位タイムアウト以外のエラー後の待機時間 */
    ulong starttime = clktime;

    /* 1. パラメタのチェック  */
#if NETHER
    if (descrp < ETH0 || descrp >= ETH0 + NETHER)
#endif
    {
        DHCP_TRACE("Bad device descriptor.\n");
        return SYSERR;
    }

    if (0 == timeout)
    {
        DHCP_TRACE("Timeout cannot be zero.\n");
        return SYSERR;
    }

    if (NULL == data)
    {
        DHCP_TRACE("No data buffer provided.\n");
        return SYSERR;
    }

    if (NULL != netLookup(descrp))
    {
        DHCP_TRACE("Network interface is up on device.\n");
        return SYSERR;
    }

    /* 2. パケットの遷移処理 */
    bzero(data, sizeof(*data));
    data->state = DHCPC_STATE_INIT;

    /* 成功するか、タイムアウトするまで処理を継続 */
    while (clktime <= starttime + timeout)
    {
        switch (data->state)
        {
        case DHCPC_STATE_INIT:
            /* 2.1 転送データの初期化してDHCPDISCOVERをブロードキャスト */

            data->cxid = rand();          /* コンテキストIDをランダムに選択 */
            data->starttime = clktime;    /* 開始時間をセット   */

            data->clientIpv4Addr = 0;     /* クライアントIPアドレスは不明  */
            data->serverIpv4Addr = 0;     /* サーバIPアドレスは不明        */

            /* クライアントのハードウェアアドレスは既知; ネットワークデバイスから取得 */
            if (SYSERR == control(descrp, ETH_CTRL_GET_MAC,
                                  (long)data->clientHwAddr, 0))
            {
                DHCP_TRACE("Failed to get client hardware address");
                return SYSERR;
            }

            /* サーバのハードウェアアドレスは不明  */
            bzero(data->serverHwAddr, ETH_ADDR_LEN);

            /* DHCPDISCOVERをブロードキャスト  */
            DHCP_TRACE("Sending DHCPDISCOVER");
            retval = dhcpSendRequest(descrp, data);

            if (OK == retval)
            {
                /* 要求に成功したら状態遷移 */
                DHCP_TRACE("Sent DHCPDISCOVER");
                data->state = DHCPC_STATE_SELECTING;
            }
            else
            {
                DHCP_TRACE("Failed to send DHCPDISCOVER; returning failure");
                return SYSERR;
            }
            break;

        case DHCPC_STATE_SELECTING:
            /* 2.2 任意のサーバからのDHCPOFFERを待機してリースを要求 */
            DHCP_TRACE("Waiting for DHCPOFFER");
            retval = dhcpRecvReply(descrp, data, recvTimeout);

            /* DHCPOFFERが得られなかった場合は初期状態に戻してdelayだけsleepして再開 */
            if (OK != retval)
            {
                DHCP_TRACE("Failed to receive DHCPOFFER");
                data->state = DHCPC_STATE_INIT;
                if (TIMEOUT != retval)
                {
                    sleep(delay);
                }
                break;
            }

            /* DHCPOFFERに基づいてサーバにDHCPREQUESTを送信する  */
            DHCP_TRACE("Sending DHCPREQUEST");
            retval = dhcpSendRequest(descrp, data);

            if (OK == retval)
            {
                /* 要求に成功したら状態遷移 */
                DHCP_TRACE("Sent DHCPREQUEST");
                data->state = DHCPC_STATE_REQUESTING;
            }
            else
            {
                /* 要求が失敗したら初期状態に戻してdelayだけsleepして再開 */
                DHCP_TRACE("Failed to send DHCPREQUEST");
                data->state = DHCPC_STATE_INIT;
                sleep(delay);
            }
            break;

        case DHCPC_STATE_REQUESTING:
            /* 2.3 DHCPREQUESTを送信したサーバからのDHCPACKを待機してACKを受信したら終了 */
            DHCP_TRACE("Waiting for DHCPACK");
            retval = dhcpRecvReply(descrp, data, recvTimeout);

            if (OK == retval)
            {
                /* ACKを受信したら成功で終了 */
                DHCP_TRACE("Received DHCPACK");
                data->state = DHCPC_STATE_BOUND;
                return OK;
            }
            else
            {
                /* 失敗したら初期状態に戻してdelayだけsleepして再開 */
                DHCP_TRACE("Failed to receive DHCPACK");
                data->state = DHCPC_STATE_INIT;
                if (TIMEOUT != retval)
                {
                    sleep(delay);
                }
            }
            break;
        }
    }
    return TIMEOUT;
}
