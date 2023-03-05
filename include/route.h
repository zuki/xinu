/**
 * @file route.h
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _ROUTE_H_
#define _ROUTE_H_

#include <stddef.h>
#include <mailbox.h>
#include <network.h>

/* Tracing macros */
//#define TRACE_RT     TTY1
#ifdef TRACE_RT
#include <stdio.h>
#define RT_TRACE(...)     { \
		fprintf(TRACE_RT, "%s:%d (%d) ", __FILE__, __LINE__, gettid()); \
		fprintf(TRACE_RT, __VA_ARGS__); \
		fprintf(TRACE_RT, "\n"); }
#else
#define RT_TRACE(...)
#endif

/** @ingroup route
 * @def RT_NENTRY
 * @brief ARPテーブルエントリ数 */
#define RT_NENTRY         32
/** @ingroup route
 * @def RT_FREE
 * @brief エントリは未使用 */
#define RT_FREE           0
/** @ingroup route
 * @def RT_USED
 * @brief エントリは使用中 */
#define RT_USED           1
/** @ingroup route
 * @def RT_PEND
 * @brief エントリは保留中 */
#define RT_PEND           2

/** @ingroup route
 * @def RT_THR_PRIO
 * @brief ルートスレッド優先度 */
#define RT_THR_PRIO        NET_THR_PRIO
/** @ingroup route
 * @def RT_THR_STK
 * @brief ルートスレッドスタックサイズ */
#define RT_THR_STK         NET_THR_STK

/** @ingroup route
 * @def RT_NQUEUE
 * キューに許されるパケット数
 */
#define RT_NQUEUE          32

/** @ingroup route
 * ルートパケット構造体
 */
struct rtEntry
{
    ushort state;               /**< 状態 */
    ushort masklen;             /**< マスク長 */
    struct netaddr dst;         /**< 宛先ネットワークアドレス */
    struct netaddr gateway;     /**< ゲートウェイネットワークアドレス */
    struct netaddr mask;        /**< サブネットマスク */
    struct netif *nif;          /**< ネットワークインタフェース */
};

/* ルートテーブル */
extern struct rtEntry rttab[RT_NENTRY];

/* ルーティングが必要なパケット用のルートパケットキュー */
extern mailbox rtqueue;

/* 関数プロトタイプ */
syscall rtAdd(const struct netaddr *dst, const struct netaddr *gate,
              const struct netaddr *mask, struct netif *nif);
struct rtEntry *rtAlloc(void);
thread rtDaemon(void);
syscall rtDefault(const struct netaddr *gate, struct netif *nif);
syscall rtInit(void);
struct rtEntry *rtLookup(const struct netaddr *addr);
syscall rtRecv(struct packet *pkt);
syscall rtRemove(const struct netaddr *dst);
syscall rtClear(struct netif *nif);
syscall rtSend(struct packet *pkt);

#endif                          /* _ROUTE_H_ */
