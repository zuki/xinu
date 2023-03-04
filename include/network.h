/*
 * @file network.h
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <stddef.h>
#include <conf.h>
#include <ethernet.h>
#include <string.h>

/** @ingroup network
 *  @{
 */

/* Tracing macros */
//#define TRACE_NET     TTY1
#ifdef TRACE_NET
#include <stdio.h>
#define NET_TRACE(...)     { \
        fprintf(TRACE_NET, "%s:%d (%d) ", __FILE__, __LINE__, gettid()); \
        fprintf(TRACE_NET, __VA_ARGS__); \
        fprintf(TRACE_NET, "\n"); }
#else
#define NET_TRACE(...)
#endif

/* Endian conversion macros*/
#if BYTE_ORDER == LITTLE_ENDIAN
/** @def hs2net(x)
 * 16bitをネットワークバイトオーダに変換する */
#define hs2net(x) (unsigned) ((((x)>>8) &0xff) | (((x) & 0xff)<<8))
/** @def net2hs(x)
 * 16bitをホストバイトオーダに変換する */
#define net2hs(x) hs2net(x)
/** @def hl2net(x)
 * 32bitをホストバイトオーダに変換する */
#define hl2net(x)   ((((x)& 0xff)<<24) | (((x)>>24) & 0xff) | \
    (((x) & 0xff0000)>>8) | (((x) & 0xff00)<<8))
/** @def net2hl(x)
 * 32bitをネットワークバイトオーダに変換する */
#define net2hl(x) hl2net(x)
#else
#define hs2net(x) (x)
#define net2hs(x) (x)
#define hl2net(x) (x)
#define net2hl(x) (x)
#endif

/** @def NET_MAX_ALEN
 * 最大ネットワークアドレス長 */
#define NET_MAX_ALEN    6

/**
 * @ingroup network
 * @brief ネットワークアドレス構造体.
 */
struct netaddr
{
    ushort type;                      /**< アドレス種別 (NETADDR_*)     */
    uchar len;                        /**< アドレス長                   */
    uchar addr[NET_MAX_ALEN];         /**< アドレス                     */
};

/* Netwok address types */

/** ネットワークアドレス種別: ethernet */
#define NETADDR_ETHERNET    1
/** ネットワークアドレス種別: IPv4 */
#define NETADDR_IPv4        ETHER_TYPE_IPv4

extern const struct netaddr NETADDR_GLOBAL_IP_BRC;
extern const struct netaddr NETADDR_GLOBAL_ETH_BRC;

/* Network address macros (実装は network/netaddr にあり)*/
bool netaddrequal(const struct netaddr *, const struct netaddr *);
syscall netaddrmask(struct netaddr *, const struct netaddr *);
syscall netaddrhost(struct netaddr *, const struct netaddr *);
/** ネットワークアドレスをコピーする */
#define netaddrcpy(dst, src)     memcpy(dst, src, sizeof(struct netaddr))
int netaddrsprintf(char *, const struct netaddr *);

/** 標準的な基礎となるネットワークデバイスドライバ制御機能 */
#define NET_GET_MTU         200     /**< MTUを取得する */
#define NET_GET_LINKHDRLEN  201     /**< リンクヘッダを取得する */
#define NET_GET_HWADDR      203     /**< ハードウェアアドレスを取得する */
#define NET_GET_HWBRC       204     /**< ハードウェアブロードキャストはドレスを取得する */

/** ネットワークアドレスインタフェース構造体宣言 */
#ifdef NETHER
#ifdef NETHLOOP
#define NNETIF    NETHER+NETHLOOP     /**< ネットワークインタフェースの数 */
#else
#define NNETIF    NETHER              /**< ネットワークインタフェースの数 */
#endif
#endif


/** ネットワーク受信スレッドの定数 */
#define NET_NTHR       5              /**< ネットワーク受信スレッドの数   */
#define NET_THR_PRIO   30             /**< ネットワーク受信スレッドの優先度 */
#define NET_THR_STK    4096           /**< ネットワーク受信スレッドのスタックサイズ */

/** ネットワークテーブルエントリの状態 */
#define NET_FREE   0                  /**< Netifは未使用             */
#define NET_ALLOC  1                  /**< Netifは割り当て済み       */

/**
 * @ingroup network
 * @brief ネットワークインタフェース制御ブロック構造体.
 */
struct netif
{
    int dev;                          /**< デバイス構造体               */
    ushort state;                     /**< テーブルエントリの状態       */
    uint mtu;                         /**< ネットワークデバイスのMTU    */
    uint linkhdrlen;                  /**< リンク層のヘッダー長         */
    struct netaddr ip;                /**< インタフェースのプロトコルアドレス   */
    struct netaddr mask;              /**< プロトコルアドレスのサブネットマスク */
    struct netaddr gateway;           /**< ゲートウェイのプロトコルアドレス     */
    struct netaddr ipbrc;             /**< ブロードキャストプロトコルアドレス   */
    struct netaddr hwaddr;            /**< ハードウェアアドレス         */
    struct netaddr hwbrc;             /**< ハードウェアブロードキャストアドレス */
    tid_typ recvthr[NET_NTHR];        /**< 受信スレッドids              */
    uint nin;                         /**< 受信パケット数               */
    uint nproc;                       /**< 処理済み受信パケット数       */
    void *capture;                    /**< Snoopキャプチャ構造体        */
};

extern struct netif netiftab[];

/* Network packet buffer pool */
extern int netpool;

/** 最大パケット長. この数のmod 4は2でなければならない */
#define NET_MAX_PKTLEN        1598
/**
 * プールサイズ. プールサイズ >= ARP_NQUEUE + RT_NQUEUE + UDP_IBLEN + RAW_IBLEN
 * でなければならない
 */
#define NET_POOLSIZE        512

/**
 * @ingroup network
 * @brief 着信パケット構造体.
 */
struct packet
{
    struct netif *nif;          /**< パケットのネットワークインタフェース */
    uint len;                   /**< 総パケット長                       */
    uchar *linkhdr;             /**< リンク層ヘッダーへのポインタ       */
    uchar *nethdr;              /**< ネットワーク層ヘッダーへのポインタ */
    uchar *curr;                /**< パケット内の現在位置へのポインタ   */
    uchar pad[2];               /**< ワードアライメントのための詰め物   */
    uchar data[1];              /**< 着信パケットへのポインタ           */
};

/* 関数プロトタイプ (実装はnetwork/net にあり） */
ushort netChksum(void *, uint);
syscall netDown(int);
syscall netFreebuf(struct packet *);
struct packet *netGetbuf(void);
syscall netInit(void);
struct netif *netLookup(int);
thread netRecv(struct netif *);
syscall netSend(struct packet *, const struct netaddr *, const struct netaddr *,
                ushort);
syscall netUp(int, const struct netaddr *, const struct netaddr *,
              const struct netaddr *);

/**  @} */

#endif                          /* _NETWORK_H_ */
