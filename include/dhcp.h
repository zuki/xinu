/**
 * @file dhcp.h
 *
 *
 */
/* Embedded Xinu, Copyright (C) 2008, 2013.  All rights reserved. */

/** @ingroup dhcpc
 *  @{
 */

#ifndef _DHCP_H_
#define _DHCP_H_

#include <stddef.h>
#include <network.h>
#include <dhcpc.h>

/* DHCPオペレーション */
#define DHCP_OP_REQUEST		1
#define DHCP_OP_REPLY		2

#define DHCP_HW_ETHERNET    1       /**< Ethernet */

#define DHCP_FLAGS_BROADCAST    (1 << 15)

/* DHCPタイムアウト定義 */
#define DHCP_RETRANSMIT_COUNT    4
#define DHCP_RETRANSMIT_TIME     5000   /* in ms */

/* DHCPクライアントの状態  */
#define DHCPC_STATE_INIT         0
#define DHCPC_STATE_SELECTING    1
#define DHCPC_STATE_REQUESTING   2
#define DHCPC_STATE_BOUND        3
#define DHCPC_STATE_RENEW        4
#define DHCPC_STATE_REBIND       5

/* DHCP Message Types */
#define DHCPDISCOVER    1
#define DHCPOFFER		2
#define DHCPREQUEST		3
#define DHCPDECLINE		4
#define DHCPACK			5
#define DHCPNAK			6
#define DHCPRELEASE		7
#define DHCP_TIMEOUT	0

/* DHCP definitions */
#define DHCP_HDR_LEN		240
#define DHCP_OMSGTYPE_LEN	3
#define DHCP_MAGICCOOKIE    0x63825363
#define DHCP_HTYPE_ETHER	1
#define DHCP_BROADCAST		0x8000

/* DHCP OPTIONS */
#define DHCP_OPT_END            255
#define DHCP_OPT_PAD            0
#define DHCP_OPT_SUBNET         1
#define DHCP_OPT_GATEWAY        3
#define DHCP_OPT_DNS            6
#define DHCP_OPT_HNAME			12
#define DHCP_OPT_DOMAIN         15
#define DHCP_OPT_REQUEST        50
#define DHCP_OPT_LEASE          51
#define DHCP_OPT_MSGTYPE        53
#define DHCP_OPT_SERVER         54
#define DHCP_OPT_PARAMREQ		55

/**
 * @ingroup dhcpc
 *
 * @brief DHCPパケット構造体.
 *
 * \code
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Ethernet Header (14 octets)                                   |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | IP Header (20 octets)                                         |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | UDP Header (8 octets)                                         |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Op            | Hardware Type | Hardware Len  | Hops          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Transaction Id                                                |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Seconds                       | Flags                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Client IP                                                     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Your IP                                                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Server IP                                                     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Gateway IP                                                    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Client Hardware (16 octets)                                   |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Server Hostname (64 octets)                                   |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Boot filename (128 octets)                                    |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | DHCP Options (Variable octets)                                |
 * | Each options has: Type (1 octet), Len (1 octet), and Value    |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \endocode
 */
struct dhcpPkt
{
    uchar op;             /**< オペレーション（要求/応答）  */
    uchar htype;          /**< ハードウェア種別 (ethernet)  */
    uchar hlen;           /**< ハードウェア長               */
    uchar hops;           /**< ホップ数                     */

    uint xid;             /**< 転送識別子                   */
    ushort secs;          /**< 開始時間からの秒数           */
    ushort flags;         /**< フラグ                       */

    uint ciaddr;          /**< クライアントのIPアドレス     */
    uint yiaddr;          /**< あなたのIPアドレス           */
    uint siaddr;          /**< サーバのIPアドレス           */
    uint giaddr;          /**< ゲートウェイのIPアドレス     */
    uchar chaddr[16];     /**< クライアントのハードウェアアドレス */

    char sname[64];       /**< サーバ名                     */
    char file[128];       /**< ファイル（拡張子用）         */
    uint cookie;          /**< マジッククッキー             */
    uchar opts[1];        /**< オプション                   */
};


//#define ENABLE_DHCP_TRACE

#ifdef ENABLE_DHCP_TRACE
#  include <stdio.h>
#  include <thread.h>
#  define DHCP_TRACE(format, ...)                                     \
do                                                                    \
{                                                                     \
    fprintf(stderr, "%s:%d (%d) ", __FILE__, __LINE__, gettid());     \
    fprintf(stderr, format, ## __VA_ARGS__);                          \
    fprintf(stderr, "\n");                                            \
} while (0)
#else
#  define DHCP_TRACE(format, ...)
#endif


/* Note: dhcpClient() は dhcpc.h で定義されている */
syscall dhcpSendRequest(int descrp, struct dhcpData *data);
syscall dhcpRecvReply(int descrp, struct dhcpData *data, uint timeout);

/**  @} */

#endif /* _DHCP_H_ */
