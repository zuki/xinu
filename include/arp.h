/**
 * @file arp.h
 * @ingroup arp
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _ARP_H_
#define _ARP_H_

#include <stddef.h>
#include <mailbox.h>
#include <network.h>

/** トレース用マクロ */
//#define TRACE_ARP     TTY1
#ifdef TRACE_ARP
#include <stdio.h>
#define ARP_TRACE(...)     { \
		fprintf(TRACE_ARP, "%s:%d (%d) ", __FILE__, __LINE__, gettid()); \
		fprintf(TRACE_ARP, __VA_ARGS__); \
		fprintf(TRACE_ARP, "\n"); }
#else
#define ARP_TRACE(...)
#endif

/* ARPハードウェアタイプ */
#define ARP_HWTYPE_ETHERNET   1            /**< ハードウェアタイプ: ethernet. */

/* ARP Protocol Types */
#define ARP_PRTYPE_IPv4       ETHER_TYPE_IPv4  /**< ARPプロトコルタイプ: IPv4     */

/* ARP Operations */
#define ARP_OP_RQST 1              /**< ARP操作: 要求           */
#define ARP_OP_REPLY 2             /**< ARP操作: 応答             */

/* ARPヘッダー */
#define ARP_CONST_HDR_LEN   8       /**< ARPヘッダー長定数 */

/* ARP Table */
#define ARP_NENTRY         32      /**< ARPテーブルエントリ数           */
#define ARP_FREE           0       /**< エントリは空き                  */
#define ARP_USED           1       /**< エントリは使用済                */
/* ARP entry is unresolved if it is USED but not RESOLVED (0b01) */
#define ARP_UNRESOLVED     1       /**< エントリは使用済だが未解決    */
/* ARP entry is resolved if it is USED and RESOLVED (0b11) */
#define ARP_RESOLVED        3      /**< エントリは使用済で解決済        */
#define ARP_NTHRWAIT        10     /**< 待機できるスレッド数            */

/* ARP Lookup */
#define ARP_MAX_LOOKUP      5     /**< パケット足りのARP検索試行回数    */
#define ARP_MSG_RESOLVED    1     /**< メッセージはARP解決を表示する    */

/* Timing info */
#define ARP_TTL_UNRESOLVED  5     /**< 未解決のエントリ用のTTL（秒単位） */
#define ARP_TTL_RESOLVED    300   /**< 解決済のエントリ用のTTL（秒単位） */

/* ARP thread constants */
#define ARP_THR_PRIO        NET_THR_PRIO   /**< ARPスレッド優先度        */
#define ARP_THR_STK         NET_THR_STK    /**< ARPスレッドスタックサイズ*/

/** ARPパケットヘッダーオフセット: SHA */
#define ARP_ADDR_SHA(arp)   0
/** ARPパケットヘッダーオフセット: SPA */
#define ARP_ADDR_SPA(arp)   (arp->hwalen)
/** ARPパケットヘッダーオフセット: DHA */
#define ARP_ADDR_DHA(arp)   (arp->hwalen + arp->pralen)
/** ARPパケットヘッダーオフセット: DPA */
#define ARP_ADDR_DPA(arp)   ((2 * arp->hwalen) + arp->pralen)

/* ARP daemon info */
#define ARP_NQUEUE          32    /**< キューの最大許容パケット数 */

/**
 * ARPパケットヘッダー.
 *
 * \code
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Link-Level Header                                             |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |        Hardware Type          |         Protocol Type         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Hardware Len | Protocol Len   |          Operation            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Source Hardware Address (SHA)                                 |
 * | Source Protocol Address (SPA)                                 |
 * | Destination Hardware Address (DHA)                            |
 * | Destination Protocol Address (DPA)                            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \endcode
 */
struct arpPkt
{
    ushort hwtype;              /**< ハードウェアタイプ              */
    ushort prtype;              /**< プロトコルタイプ                */
    uchar hwalen;               /**< ハードウェアアドレス長          */
    uchar pralen;               /**< プロトコルアドレス長            */
    ushort op;                  /**< 操作                            */
    uchar addrs[1];             /**< 送信元と宛先のハードウェア&プロトコルアドレス */
};

/** ARPテーブルエントリ */
struct arpEntry
{
    ushort state;                    /**< ARPエントリの状態             */
    struct netif *nif;               /**< ネットワークインタフェース    */
    struct netaddr hwaddr;           /**< ハードウェアアドレス          */
    struct netaddr praddr;           /**< プロトコルアドレス            */
    uint expires;                    /**< エントリ失効時のlktime        */
    tid_typ waiting[ARP_NTHRWAIT];   /**< エントリ待機スレッド          */
    int count;                       /**< 待機スレッド数                */
};

/** ARPテーブル */
extern struct arpEntry arptab[ARP_NENTRY];

/** 応答を要求しているパケット用のARPパケットキュー */
extern mailbox arpqueue;

struct arpEntry *arpAlloc(void);
thread arpDaemon(void);
struct arpEntry *arpGetEntry(const struct netaddr *);
syscall arpFree(struct arpEntry *);
syscall arpInit(void);
syscall arpLookup(struct netif *, const struct netaddr *, struct netaddr *);
syscall arpNotify(struct arpEntry *, message);
syscall arpRecv(struct packet *);
syscall arpSendRqst(struct arpEntry *);
syscall arpSendReply(struct packet *);

#endif                          /* _ARP_H_ */
