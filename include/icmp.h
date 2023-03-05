/**
 * @file icmp.h
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _ICMP_H_
#define _ICMP_H_

#include <stddef.h>
#include <network.h>
#include <mailbox.h>
#include <route.h>

/* Tracing macros */
//#define TRACE_ICMP     TTY1
#ifdef TRACE_ICMP
#include <stdio.h>
#define ICMP_TRACE(...)     { \
		fprintf(TRACE_ICMP, "%s:%d (%d) ", __FILE__, __LINE__, gettid()); \
		fprintf(TRACE_ICMP, __VA_ARGS__); \
		fprintf(TRACE_ICMP, "\n"); }
#else
#define ICMP_TRACE(...)
#endif

/** @ingroup icmp
 * @def ICMP_THR_PRIO
 * @brief ICMPスレッド定数: 優先度 */
#define ICMP_THR_PRIO       NET_THR_PRIO
/** @ingroup icmp
 * @def ICMP_THR_STK
 * @brief ICMPスレッド定数: スタックサイズ */
#define ICMP_THR_STK        NET_THR_STK

/** @ingroup icmp
 * @def ICMP_NQUEUE
 * @brief ICMPデーモン情報: キューに許されるパケット数 */
#define ICMP_NQUEUE          10

/** @ingroup icmp
 * @def ICMP_RQST_LEN
 * @brief ICMPリクエスト長 */
#define ICMP_RQST_LEN       16
/** @ingroup icmp
 * @def ICMP_HEADER_LEN
 * @brief ICMPヘッダー長. データは含まれない */
#define ICMP_HEADER_LEN		4
/** @ingroup icmp
 * @def ICMP_DEF_DATALEN
 * @brief ICMPデフォルトデータ長.  */
#define ICMP_DEF_DATALEN    64
/** @ingroup icmp
 * @def PING_TIMEOUT
 * @brief PINGタイムアウト.  */
#define PING_TIMEOUT        1000
/** @ingroup icmp
 * @def ICMP_MAX_PINGS
 * @brief 最大PING回数.  */
#define ICMP_MAX_PINGS      10000

/** @ingroup icmp
 * @def ICMP_ECHOREPLY
 * @brief ICMPメッセージタイプ: エコー応答 */
#define ICMP_ECHOREPLY      0
/** @ingroup icmp
 * @def ICMP_UNREACH
 * @brief ICMPメッセージタイプ: 宛先到達不可 */
#define ICMP_UNREACH        3
/** @ingroup icmp
 * @def ICMP_SRCQNCH
 * @brief ICMPメッセージタイプ: クエンチ */
#define ICMP_SRCQNCH        4
/** @ingroup icmp
 * @def ICMP_REDIRECT
 * @brief ICMPメッセージタイプ: リダイレクト */
#define ICMP_REDIRECT       5
/** @ingroup icmp
 * @def ICMP_ECHO
 * @brief ICMPメッセージタイプ: エコー要求 */
#define ICMP_ECHO           8
/** @ingroup icmp
 * @def ICMP_TIMEEXCD
 * @brief ICMPメッセージタイプ: 時間超過 */
#define ICMP_TIMEEXCD       11
/** @ingroup icmp
 * @def ICMP_PARAMPROB
 * @brief ICMPメッセージタイプ: パラメタ問題 */
#define ICMP_PARAMPROB      12
/** @ingroup icmp
 * @def ICMP_TMSTMP
 * @brief ICMPメッセージタイプ: タイムスタンプ */
#define ICMP_TMSTMP         13
/** @ingroup icmp
 * @def ICMP_TMSTMPREPLY
 * @brief ICMPメッセージタイプ: タイムスタンプ応答 */
#define ICMP_TMSTMPREPLY    14
/** @ingroup icmp
 * @def ICMP_INFORQST
 * @brief ICMPメッセージタイプ: 情報要求 */
#define ICMP_INFORQST       15
/** @ingroup icmp
 * @def ICMP_INFOREPLY
 * @brief ICMPメッセージタイプ: 情報応答 */
#define ICMP_INFOREPLY      16
/** @ingroup icmp
 * @def ICMP_TRACEROUTE
 * @brief ICMPメッセージタイプ: トレースルート */
#define ICMP_TRACEROUTE     30

/* Message Codes Based Message Types */

// ICMP宛先到達不可理由コード

/** @ingroup icmp
 * @def ICMP_NET_UNR
 * @brief ICMP宛先到達不可: ネットワーク到達不可 */
#define ICMP_NET_UNR	0
/** @ingroup icmp
 * @def ICMP_HST_UNR
 * @brief ICMP宛先到達不可: ホスト到達不可 */
#define ICMP_HST_UNR	1
/** @ingroup icmp
 * @def ICMP_PROTO_UNR
 * @brief ICMP宛先到達不可: プロトコル到達不可 */
#define ICMP_PROTO_UNR	2
/** @ingroup icmp
 * @def ICMP_PORT_UNR
 * @brief ICMP宛先到達不可: ポート到達不可 */
#define ICMP_PORT_UNR	3
/** @ingroup icmp
 * @def ICMP_FOFF_DFSET
 * @brief ICMP宛先到達不可: フラグメント */
#define ICMP_FOFF_DFSET	4
/** @ingroup icmp
 * @def ICMP_SRCRT_FAIL
 * @brief ICMP宛先到達不可: ソースルートが失敗 */
#define ICMP_SRCRT_FAIL	5

//No alternate codes for Source Quench

// リダイレクトメッセージコード
/** @ingroup icmp
 * @def ICMP_RNET
 * @brief リダイレクト: ネットワークへのリダイレクト */
#define ICMP_RNET		0
/** @ingroup icmp
 * @def ICMP_RHST
 * @brief リダイレクト: ホストへのリダイレクト */
#define ICMP_RHST		1
/** @ingroup icmp
 * @def ICMP_RTOS_NET
 * @brief リダイレクト: TOS. ネットワークへのリダイレクト */
#define ICMP_RTOS_NET	2
/** @ingroup icmp
 * @def ICMP_RTOS_HST
 * @brief リダイレクト: TOS, ホストへのリダイレクト */
#define ICMP_RTOS_HST	3


//No alternate codes for Echo Request

// 時間超過コード
/** @ingroup icmp
 * @def ICMP_TTL_EXC
 * @brief 時間超過: 転送中の生存時間=0 */
#define ICMP_TTL_EXC	0
/** @ingroup icmp
 * @def ICMP_FRA_EXC
 * @brief 時間超過: リアセンブル中の生存時間=0 */
#define ICMP_FRA_EXC	1

// パラメタ問題コード
/** @ingroup icmp
 * @def ICMP_SEE_PNTR
 * @brief パラメタ問題: キャッチオールエラー */
#define ICMP_SEE_PNTR		0
/** @ingroup icmp
 * @def ICMP_MISSING_OPT
 * @brief パラメタ問題: 必要なパラメタの欠損 */
#define ICMP_MISSING_OPT	1
/** @ingroup icmp
 * @def ICMP_SEE_PNTR
 * @brief パラメタ問題: 不正な長さ */
#define ICMP_BAD_LENGTH		2

//No alternate codes for Timestamp

//No alternate codes for Timestamp Reply

//No alternate codes for Information Request

//No alternate codes for Information Reply

// トレースルートコード
/** @ingroup icmp
 * @def OP_FORWARD
 * @brief トレースルート: 転送 */
#define OP_FORWARD	0
/** @ingroup icmp
 * @def OP_FORWARD
 * @brief トレースルート: 失敗 */
#define OP_FAILED	1

/** @ingroup icmp
 * @def TRACE_DEFAULT_TTL
 * @brief デフォルトトレースルートTTL */
#define TRACE_DEFAULT_TTL 30

/**
 * @ingroup icmp
 * @brief ICMPパケット構造体.
 *
 * @verbatim
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Link-Level Header                                             |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | IP Header                                                     |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Type          | Code          | Checksum                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Data (Variable octets                                         |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @endverbatim
 */
struct icmpPkt
{
    uchar type;                 /**< ICMPタイプ           */
    uchar code;                 /**< ICMPコード           */
    ushort chksum;              /**< ICMPチェックサム     */
    uchar data[1];              /**< ICMPデータ           */
};

/**
 * @ingroup icmp
 * @brief ICMPエコー構造体
 */
struct icmpEcho
{
    ushort id;                      /**< ID */
    ushort seq;                     /**< 一連番号 */
    ulong timesec;                  /**< 秒単位のクロック時間 */
    ulong timetic;                  /**< ティック単位のクロック時間   */
    ulong timecyc;                  /**< サイクル単位のクロック時間   */
    ulong arrivsec;                 /**< 秒単位の到着時間 */
    ulong arrivtic;                 /**< ティック単位の到着時間   */
    ulong arrivcyc;                 /**< サイクル単位の到着時間   */
};

/** @ingroup icmp
 * @def NPINGQUEUE
 * @brief PINGキューのエントリ数 */
#define NPINGQUEUE 5
/** @ingroup icmp
 * @def NPINGHOLD
 * @brief PINGホールド数 */
#define NPINGHOLD  10

/**
 * @ingroup icmp
 * @brief ICMPエコーキュー構造体
 */
struct icmpEchoQueue
{
    tid_typ tid;        /**< ICMPエコー要求を送信したスレッドのID  */
    int head;           /**< 次のICMPエコー応答を格納する位置      */
    int tail;           /**< 取得する次のICMPエコー等々の位置      */
    struct packet *pkts[NPINGHOLD]; /*< 格納されたICMPエコー応答   */
};

extern struct icmpEchoQueue echotab[NPINGQUEUE];

/* ICMP echo request queue */
extern mailbox icmpqueue;

/* ICMP Function Prototypes */
thread icmpDaemon(void);
syscall icmpInit(void);
syscall icmpRecv(struct packet *);
syscall icmpDestUnreach(const struct packet *, uchar);
syscall icmpEchoRequest(struct netaddr *dst, ushort id, ushort seq);
syscall icmpEchoReply(struct packet *request);
syscall icmpRedirect(struct packet *, uchar code, struct rtEntry *);
syscall icmpSend(struct packet *pkt, uchar type, uchar code,
                 uint datalen, struct netaddr *src, struct netaddr *dst);
syscall icmpTimeExceeded(struct packet *, uchar code);

#endif                          /* _NET_H_ */
