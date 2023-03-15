/**
 * @file udp.h
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _UDP_H_
#define _UDP_H_

#include <stddef.h>
#include <network.h>
#include <ipv4.h>
#include <semaphore.h>
#include <stdarg.h>

/* Tracing macros */
//#define TRACE_UDP     TTY1
#ifdef TRACE_UDP
#include <stdio.h>
#define UDP_TRACE(...)     { \
        fprintf(TRACE_UDP, "%s:%d (%d) ", __FILE__, __LINE__, gettid()); \
        fprintf(TRACE_UDP, __VA_ARGS__); \
        fprintf(TRACE_UDP, "\n"); }
#else
#define UDP_TRACE(...)
#endif

/* UDP 定義 */
/** @ingroup udpinternal
 * @def UDP_HDR_LEN
 * @brief UDPヘッダ長 */
#define UDP_HDR_LEN	        8
/** @ingroup udpinternal
 * @def UDP_MAX_PKTS
 * @brief UDP最大パケット長 */
#define UDP_MAX_PKTS        100
/** @ingroup udpinternal
 * @def UDP_MAX_DATALEN
 * @brief UDP最大データ長 */
#define UDP_MAX_DATALEN     1024
/** @ingroup udpinternal
 * @def UDP_TTL
 * @brief UDP TTL */
#define UDP_TTL             64

/* UDP standard ports */
/** @ingroup udpexternal
 * @def UDP_PORT_RDATE
 * @brief UDP標準ポート番号: RDATE */
#define UDP_PORT_RDATE  	37
/** @ingroup udpexternal
 * @def UDP_PORT_DNS
 * @brief UDP標準ポート番号: DNS */
#define UDP_PORT_DNS		53
/** @ingroup udpexternal
 * @def UDP_PORT_DHCPS
 * @brief UDP標準ポート番号: DHCPサーバ */
#define UDP_PORT_DHCPS  	67
/** @ingroup udpexternal
 * @def UDP_PORT_DHCPC
 * @brief UDP標準ポート番号: DHCPクライアント */
#define UDP_PORT_DHCPC  	68
/** @ingroup udpexternal
 * @def UDP_PORT_TFTP
 * @brief UDP標準ポート番号: TFTP */
#define UDP_PORT_TFTP       69
/** @ingroup udpexternal
 * @def UDP_PORT_TRACEROUTE
 * @brief UDP標準ポート番号: TRACEROUTE */
#define UDP_PORT_TRACEROUTE	33434

/* UDP flags */
/** @ingroup udpexternal
 * @def UDP_FLAG_PASSIVE
 * @brief UDPフラグ: PASSIVE */
#define UDP_FLAG_PASSIVE    0x01
/** @ingroup udpexternal
 * @def UDP_FLAG_NOBLOCK
 * @brief UDPフラグ: NOBLOCK */
#define UDP_FLAG_NOBLOCK    0x02
/** @ingroup udpexternal
 * @def UDP_FLAG_BINDFIRST
 * @brief UDPフラグ: BINDFIRST */
#define UDP_FLAG_BINDFIRST  0x04

/* UDP control functions */
/** @ingroup udpexternal
 * @def UDP_CTRL_ACCEPT
 * @brief UDP制御関数: ローカルポートとIPアドレスをセットする */
#define UDP_CTRL_ACCEPT     1
/** @ingroup udpexternal
 * @def UDP_CTRL_BIND
 * @brief UDP制御関数: リモートポートとIPアドレスをセットする */
#define UDP_CTRL_BIND       2
/** @ingroup udpexternal
 * @def UDP_CTRL_CLRFLAG
 * @brief UDP制御関数: フラグをクリアする */
#define UDP_CTRL_CLRFLAG    3
/** @ingroup udpexternal
 * @def UDP_CTRL_SETFLAG
 * @brief UDP制御関数: フラグをセットする */
#define UDP_CTRL_SETFLAG    4

/* UDP state constants */
/** @ingroup udpinternal
 * @def UDP_FREE
 * @brief UDP状態定数: 未使用 */
#define UDP_FREE      0
/** @ingroup udpinternal
 * @def UDP_ALLOC
 * @brief UDP状態定数: 割当済み */
#define UDP_ALLOC     1
/** @ingroup udpinternal
 * @def UDP_OPEN
 * @brief UDP状態定数: オープン */
#define UDP_OPEN      2

/* Local port allocation ranges */
/** @ingroup udpinternal
 * @def UDP_PSTART
 * @brief ローカルポート割当範囲: 開始ポート */
#define UDP_PSTART  10000
/** @ingroup udpinternal
 * @def UDP_PMAX
 * @brief ローカルポート割当範囲: 最大ポート */
#define UDP_PMAX    65000   /**< max UDP port */

#ifndef __ASSEMBLER__

/**
 * @ingroup udpinternal
 * @struct udpPkt
 * @brief UDPパケット構造体.
 *
 * @code
 * UDP HEADER
 *
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Ethernet Header (14 octets)                                   |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | IP Header (20 octets)                                         |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Source Port                   | Destination Port              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Message Length                | Checksum                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Data (Variable octets)                                        |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @endcode
 */
struct udpPkt
{
    ushort srcPort;             /**< UDP送信元ポート                 */
    ushort dstPort;             /**< UDPあて先ポート                 */
    ushort len;                 /**< UDPパケット長（ヘッダーを含む） */
    ushort chksum;              /**< UDPチェックサム                 */
    uchar data[1];              /**< UDPデータ                       */
};

/**
 * @ingroup udpinternal
 * @struct udpPseudoHdr
 * @brief UDP疑似ヘッダー構造体. チェックサムの計算に使用
 *
 * @code
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Source IP Address                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Destination IP Address                                        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Zero          | Protocol        | UDP Length                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @endcode
 */
struct udpPseudoHdr
{
    uchar srcIp[IPv4_ADDR_LEN];     /**< 送信元IPアドレス            */
    uchar dstIp[IPv4_ADDR_LEN];     /**< あて先IPアドレス            */
    uchar zero;                     /**< すべてゼロのフィールド      */
    uchar proto;                    /**< 使用するプロトコル (UDP=17) */
    ushort len;                     /**< UDPパケット長               */
};

/**
 * @ingroup udpinternal
 * @struct udp
 * @brief UDP制御ブロック構造体.
 */
struct udp
{
    device *dev;                        /**< UDPデバイスエントリ      */
    struct udpPkt *in[UDP_MAX_PKTS];    /**< 格納パケットへのポインタ */
    int inPool;                         /**< 受信UDPパケットのプール  */
    int icount;                         /**< 入力バッファのカウント値 */
    int istart;                         /**< 入力バッファの開始値     */
    semaphore isem;                     /**< 入力バッファ用のセマフォ */

    ushort localpt;                     /**< UDPローカルポート        */
    ushort remotept;                    /**< UDPリモートポート        */
    struct netaddr localip;             /**< UDPローカルIPアドレス    */
    struct netaddr remoteip;            /**< UDPリモートIPアドレス    */

    uchar state;                        /**< UDP状態                  */
    uchar flags;                        /**< UDPフラグ                */
};

extern struct udp udptab[];

/* 関数プロトタイプ */
devcall udpInit(device *);
devcall udpOpen(device *, va_list);
devcall udpClose(device *);
devcall udpRead(device *, void *, uint);
devcall udpWrite(device *, const void *, uint);
ushort udpAlloc(void);
ushort udpChksum(struct packet *, ushort, const struct netaddr *,
                 const struct netaddr *);
struct udp *udpDemux(ushort, ushort, const struct netaddr *,
                     const struct netaddr *);
syscall udpRecv(struct packet *, const struct netaddr *,
                const struct netaddr *);
syscall udpSend(struct udp *, ushort, const void *);
devcall udpControl(device *, int, long, long);
struct udpPkt *udpGetbuf(struct udp *);
syscall udpFreebuf(struct udpPkt *);

#endif                          /* __ASSEMBLER__ */

#endif                          /* _UDP_H_ */
