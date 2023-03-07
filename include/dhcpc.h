/**
 * @file dhcpc.h
 *
 * DHCP client interface
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#ifndef _DHCPC_H_
#define _DHCPC_H_

#include <stddef.h>
#include <network.h>

/**
 * @ingroup dhcpc
 * DHCP転送データ構造体. */
struct dhcpData
{
    /* DHCP client output */
    struct netaddr ip;              /**< クライアントIPアドレス */
    struct netaddr gateway;         /**< ゲートウェイアドレス */
    struct netaddr mask;            /**< サブネットマスク */
    struct netaddr dns;             /**< DNSサーバアドレス */
    char bootfile[128];             /**< ブートファイル名 */
    struct netaddr next_server;     /**< 次のサーバのIPアドレス */

    /* DHCP client internal variables */
    int state;                          /**< 状態 */
    uint cxid;                          /**< コンテキストID */
    uint starttime;                     /**< 開始時間 */
    int recvStatus;                     /**< 受信状態 */
    ushort discoverSecs;                /**< 発見時間 */
    uint offeredIpv4Addr;               /**< 提案IPv4アドレス */
    uint clientIpv4Addr;                /**< クライアントIPv4アドレス */
    uint serverIpv4Addr;                /**< サーバIPV4アドレス */
    uchar clientHwAddr[ETH_ADDR_LEN];   /**< クライアントハードウェアアドレス */
    uchar serverHwAddr[ETH_ADDR_LEN];   /**< サーバハードウェアアドレス */
};

syscall dhcpClient(int descrp, uint timeout, struct dhcpData *data);

#endif /* _DHCPC_H_ */
