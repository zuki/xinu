/**
 * @file telnet.h
 */
/* Embedded Xinu, Copyright (C) 2009. All rights reserved. */

#ifndef _TELNET_H_
#define _TELNET_H_

#include <device.h>
#include <stdarg.h>
#include <stddef.h>
#include <semaphore.h>
#include <thread.h>
#include <network.h>

/* Testing macros */
//#define TRACE_TELNET CONSOLE
#ifdef TRACE_TELNET
#include <stdio.h>
#define TELNET_TRACE(...)     { \
		fprintf(TRACE_TELNET, "%s:%d (%d) ", __FILE__, __LINE__, gettid()); \
		fprintf(TRACE_TELNET, __VA_ARGS__); \
		fprintf(TRACE_TELNET, "\n"); }
#else
#define TELNET_TRACE(...)
#endif

/** @ingroup telnet
 * @def TELNET_PORT
 * @brief デフォルト telnet ポート */
#define TELNET_PORT     23
/** @ingroup telnet
 * @def TELNET_IBLEN
 * @brief 入力バッファ長 */
#define TELNET_IBLEN    80
/** @ingroup telnet
 * @def TELNET_OBLEN
 * @brief 出力バッファ長 */
#define TELNET_OBLEN    80

/* Telnet Codes */
/** @ingroup telnet
 * @def TELNET_EOR
 * @brief end of record コマンド */
#define TELNET_EOR      239
/** @ingroup telnet
 * @def TELNET_SE
 * @brief end of subnegotiationsコマンド */
#define TELNET_SE       240
/** @ingroup telnet
 * @def TELNET_NOP
 * @brief no operation コマンド */
#define TELNET_NOP      241
/** @ingroup telnet
 * @def TELNET_DM
 * @brief data mark コマンド  */
#define TELNET_DM       242
/** @ingroup telnet
 * @def TELNET_BRK
 * @brief break NVT文字 */
#define TELNET_BRK      243
/** @ingroup telnet
 * @def TELNET_IP
 * @brief interupt process コマンド */
#define TELNET_IP       244
/** @ingroup telnet
 * @def TELNET_AO
 * @brief abort output コマンド */
#define TELNET_AO       245
/** @ingroup telnet
 * @def TELNET_AYT
 * @brief are you there コマンド */
#define TELNET_AYT      246
/** @ingroup telnet
 * @def TELNET_EC
 * @brief erase character コマンド */
#define TELNET_EC       247
/** @ingroup telnet
 * @def TELNET_EL
 * @brief erase line コマンド */
#define TELNET_EL       248
/** @ingroup telnet
 * @def TELNET_GA
 * @brief go ahead コマンド */
#define TELNET_GA       249
/** @ingroup telnet
 * @def TELNET_SB
 * @brief begin option subnegotiateions コマンド */
#define TELNET_SB       250
/** @ingroup telnet
 * @def TELNET_WILL
 * @brief will enable コマンド */
#define TELNET_WILL     251
/** @ingroup telnet
 * @def TELNET_WONT
 * @brief won't enable コマンド */
#define TELNET_WONT     252
/** @ingroup telnet
 * @def TELNET_DO
 * @brief request other party enable オプション */
#define TELNET_DO       253
/** @ingroup telnet
 * @def TELNET_DONT
 * @brief request other party doesn't enable オプション */
#define TELNET_DONT     254
/** @ingroup telnet
 * @def TELNET_IAC
 * @brief コマンドとして解釈する */
#define TELNET_IAC      255

/* special characters */

/** @ingroup telnet
 * @def TELNET_CHR_CLOSE
 * @brief 特殊文字: クローズ */
#define TELNET_CHR_CLOSE            29
/** @ingroup telnet
 * @def TELNET_CHR_EL
 * @brief 特殊文字: ライン消去 */
#define TELNET_CHR_EL               21
/** @ingroup telnet
 * @def TELNET_CHR_DEL
 * @brief 特殊文字: 削除 */
#define TELNET_CHR_DEL              127


/* TELNET options */
/** @ingroup telnet
 * @def TELNET_ECHO
 * @brief echo オプションコード */
#define TELNET_ECHO             1
/** @ingroup telnet
 * @def TELNET_SUPPRESS_GA
 * @brief suppress go ahead オプションコード */
#define TELNET_SUPPRESS_GA      3
/** @ingroup telnet
 * @def TELNET_TRANSMIT_BINARY
 * @brief transmit binary オプションコード */
#define TELNET_TRANSMIT_BINARY  0

/* TELNET flags */
/** @ingroup telnet
 * @def TELNET_FLAG_ECHO
 * @brief echo フラグ */
#define TELNET_FLAG_ECHO            0x01
/** @ingroup telnet
 * @def TELNET_FLAG_SUPPRESS_GA
 * @brief suppress go ahead フラグ */
#define TELNET_FLAG_SUPPRESS_GA     0x02
/** @ingroup telnet
 * @def TELNET_FLAG_TRANSMIT_BINARY
 * @brief transmit binary フラグ */
#define TELNET_FLAG_TRANSMIT_BINARY 0x04

/* Control funcitons */
/** @ingroup telnet
 * @def TELNET_CTRL_FLUSH
 * @brief 制御機能: 出力バッファのフラッシュ */
#define TELNET_CTRL_FLUSH       1
/** @ingroup telnet
 * @def TELNET_CTRL_CLRFLAG
 * @brief 制御機能: フラグのクリア */
#define TELNET_CTRL_CLRFLAG     2
/** @ingroup telnet
 * @def TELNET_CTRL_SETFLAG
 * @brief 制御機能: フラグのセット */
#define TELNET_CTRL_SETFLAG     3

/* TELNET device states */
/** @ingroup telnet
 * @def TELNET_STATE_FREE
 * @brief デバイス状態; 未使用 */
#define TELNET_STATE_FREE       0
/** @ingroup telnet
 * @def TELNET_STATE_ALLOC
 * @brief デバイス状態; 割り当て */
#define TELNET_STATE_ALLOC      1
/** @ingroup telnet
 * @def TELNET_STATE_OPEN
 * @brief デバイス状態; オープン */
#define TELNET_STATE_OPEN       2

/* TELNET echo negotation states */
/** @ingroup telnet
 * @def TELNET_ECHO_START
 * @brief エコーネゴシエーション状態: START */
#define TELNET_ECHO_START           0
/** @ingroup telnet
 * @def TELNET_ECHO_SENT_DO
 * @brief エコーネゴシエーション状態: SENT_DO */
#define TELNET_ECHO_SENT_DO         1
/** @ingroup telnet
 * @def TELNET_ECHO_SENT_WILL
 * @brief エコーネゴシエーション状態: SENT_WILL */
#define TELNET_ECHO_SENT_WILL       2
/** @ingroup telnet
 * @def TELNET_ECHO_SELF_ECHOES
 * @brief エコーネゴシエーション状態: SELF_ECHOES */
#define TELNET_ECHO_SELF_ECHOES     3
/** @ingroup telnet
 * @def TELNET_ECHO_OTHER_ECHOES
 * @brief エコーネゴシエーション状態: OTHER_ECHOES */
#define TELNET_ECHO_OTHER_ECHOES    4
/** @ingroup telnet
 * @def TELNET_ECHO_NO_ECHO
 * @brief エコーネゴシエーション状態: NO_ECHO */
#define TELNET_ECHO_NO_ECHO         5

/** @ingroup telnet
 * @struct telnet
 * @brief telnet構造体 */
struct telnet
{
    /* Pointers to associated structures */
    device *phw;                /**< ハードウェアデバイス構造体         */

    /* TELNET fields */
    uchar state;                /**< 状態: TELNET_STATE_*               */
    uchar flags;                /**< オプションフラグ                   */
    uchar echoState;            /**< echoオプションネゴシエーション状態 */
    semaphore killswitch;       /**< サーバスレッドをkillするためのセマフォ */

    /* TELNET input fields */
    bool ieof;                  /**< 入力バッファがEOFか                */
    bool idelim;                /**< バッファの部分ラインか             */
    char in[TELNET_IBLEN];      /**< 入力バッファ                       */
    uint icount;                /**< 入力バッファ内の文字数             */
    uint istart;                /**< 入力バッファの先頭文字のIndex      */
    semaphore isem;             /**< 入力バッファ用のセマフォ           */

    /* TELNET output fields */
    char out[TELNET_OBLEN];     /**< 出力バッファ                       */
    uint ocount;                /**< 出力バッファ内の文字数             */
    uint ostart;                /**< 出力バッファの先頭文字のIndex      */
    semaphore osem;             /**< 出力バッファ用のセマフォ           */
};

extern struct telnet telnettab[];

/* Driver functions */
int telnetAlloc(void);
devcall telnetInit(device *);
devcall telnetOpen(device *, va_list);
devcall telnetClose(device *);
devcall telnetRead(device *, void *, uint);
devcall telnetWrite(device *, void *, uint);
devcall telnetGetc(device *);
devcall telnetPutc(device *, char);
devcall telnetControl(device *, int, long, long);
devcall telnetFlush(device *);
thread telnetServer(int, int, ushort, char *);

#endif                          /* _TELNET_H_ */
