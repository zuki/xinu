/**
 * @file ether.h
 *
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#ifndef _ETHER_H_
#define _ETHER_H_

#include <device.h>
#include <ethernet.h>
#include <stdarg.h>
#include <stddef.h>
#include <semaphore.h>

/* トレースマクロ */
//#define TRACE_ETHER   TTY1
#ifdef TRACE_ETHER
#include <stdio.h>
#define ETHER_TRACE(...)     { \
        fprintf(TRACE_ETHER, "%s:%d (%d) ", __FILE__, __LINE__, gettid()); \
        fprintf(TRACE_ETHER, __VA_ARGS__); \
        fprintf(TRACE_ETHER, "\n"); }
#else
#define ETHER_TRACE(...)
#endif

#define ETH_ADDR_LEN        6   /**< Ethernetアドレス長                 */

#include <vlan.h>

/* ETHバファ長 */
#define ETH_IBLEN           1024 /**< 入力バッファサイズ                */

/* Ethernet DMA バッファサイズ */
#define ETH_MTU             1500 /**< MTU（最大転送ユニット）           */
#define ETH_HEADER_LEN      ETH_HDR_LEN  /**< Ethernetヘッダー長        */
#define ETH_VLAN_LEN        4   /**< Ethernet vlanタグ長                */
#define ETH_CRC_LEN         4   /**< Ethernet CRC長                     */
#define ETH_MAX_PKT_LEN     ( ETH_HEADER_LEN + ETH_VLAN_LEN + ETH_MTU )

#define ETH_RX_BUF_SIZE     ( ETH_MAX_PKT_LEN + ETH_CRC_LEN \
                              + sizeof(struct rxHeader) )
#define ETH_TX_BUF_SIZE     ( ETH_MAX_PKT_LEN )

/* ETH状態 */
#define ETH_STATE_FREE       0
#define ETH_STATE_DOWN       1
#define ETH_STATE_UP         2

/* ETH制御コード */
#define ETH_CTRL_CLEAR_STATS 1  /**< Ethernet統計をクリアする           */
#define ETH_CTRL_SET_MAC     2  /**< このデバイスのMACをセットする      */
#define ETH_CTRL_GET_MAC     3  /**< このデバイスのMACを取得する        */
#define ETH_CTRL_SET_LOOPBK  4  /**< ループバックモードをセットする     */
#define ETH_CTRL_RESET       5  /**< Ethernetデバイスをリセットする     */
#define ETH_CTRL_DISABLE     6  /**< Ethernetデバイスを無効にする       */

/**
 * Ethernetパケットバッファ
 */
struct ethPktBuffer
{
    uchar *buf;                 /**< バッファへのポインタ               */
    uchar *data;                /**< バッファ内のデータの開始地点       */
    int length;                 /**< パケットデータ長                   */
};

/* Ethernetコントロールブロック */
#define ETH_INVALID  (-1)       /**< 不正なデータ（仮想デバイス）       */

/**
 * Ethernetコントロールブロック
 */
struct ether
{
    uchar state;                /**< 状態: ETH_STATE_*                  */
    device *phy;                /**< Tx DMA用の物理ethデバイス          */

    /* 関連する構造体へのポインタ */
    device *dev;                /**< ethデバイス構造体                  */
    void *csr;                  /**< 制御及びステータスレジスタ         */

    ulong interruptMask;        /**< 割り込みマスク                     */
    ulong interruptStatus;      /**< 割り込みステータス                 */

    struct dmaDescriptor *rxRing; /**< 受信リングディスクリプタの配列   */
    struct ethPktBuffer **rxBufs; /**< Rxリング配列                     */
    ulong rxHead;               /**< Rxリングの先頭のインデックス       */
    ulong rxTail;               /**< Rxリングの末尾のインデックス       */
    ulong rxRingSize;           /**< Rxリングディクリプタの数           */
    ulong rxirq;                /**< Rx割り込み要求の回数               */
    ulong rxOffset;             /**< rxHeaderのバイト長                 */
    ulong rxErrors;             /**< Rxエラー回数                       */

    struct dmaDescriptor *txRing; /**< 送信リングディスクリプタの配列   */
    struct ethPktBuffer **txBufs; /**< Tx リング配列                    */
    ulong txHead;               /**< Txリングの先頭のインデックス       */
    ulong txTail;               /**< Txリングの末尾のインデックス       */
    ulong txRingSize;           /**< Txリングディクリプタの数           */
    ulong txirq;                /**< Tx割り込み要求の回数               */

    uchar devAddress[ETH_ADDR_LEN];

    uchar addressLength;        /**< ハードウェアアドレス長             */
    ushort mtu;                 /**< MTU (Maximum transmission units)   */

    ulong errors;               /**< Ethernetエラー数                   */
    ushort ovrrun;              /**< バッファオーバーラン               */
    semaphore isema;            /**< eth入力用のI/0セマフォ             */
    ushort istart;              /**< 最初のバイトのインデックス         */
    ushort icount;              /**< バッファ内のパケット               */

    struct ethPktBuffer *in[ETH_IBLEN]; /**< 入力バッファ               */

    int inPool;                 /**< 入力用のバッファプールID           */
    int outPool;                /**< 出力用のバッファプールID           */
};

/**
 * \ingroup etherdriver
 *
 * Ethernetデバイスのグローバルテーブル.  Xinuは複数のタイプのEthernet
 * デバイスを同時にはサポートしておらず、すべて同じ種類しかサポートして
 * いないことに注意されたい。
 */
extern struct ether ethertab[];

/* Ethernetドライバ関数 */

/**
 * \ingroup etherdriver
 *
 * Ethernetデバイスの一回限りの初期化を行う.  Ethernetコントロールブロック
 * の初期化やハードウェアパラメータの設定など、etherOpen() の呼び出しから
 * etherClose() の呼び出しまで存在するthernetデバイスの初期化を行う。
 *
 * @param devptr
 *      このethernetデバイス用のXinuのデバイステーブルのエントリへのポインタ
 * @return
 *      デバイスの初期化に成功したら ::OK; それ以外は ::SYSERR.
 */
devcall etherInit(device *devptr);

/**
 * \ingroup etherdriver
 *
 * Ethernetデバイスをオープンする.   TxとRxの機能を有効にし、デバイスの
 * 状態を ::ETH_STATE_UP に設定する。 open() 関数から呼び出す必要が
 * ある。
 *
 * @param devptr
 *      このethernetデバイス用のXinuのデバイステーブルのエントリへのポインタ
 *
 * @return
 *      デバイスのオープンに成功したら ::OK; それ以外は ::SYSERR.
 */
devcall etherOpen(device *devptr);

/**
 * \ingroup etherdriver
 *
 * Ethernetデバイスをクローズする。 TxとRxの機能を無効にし、デバイスの
 * 状態を ::ETH_STATE_DOWN に設定する。 close() 関数から呼び出す必要が
 * ある。
 *
 * @param devptr
 *     このethernetデバイス用のXinuのデバイステーブルのエントリへのポインタ
 *
 * @return デバイスのクローズが成功したら ::OK; それ以外は ::SYSERR.
 */
devcall etherClose(device *devptr);

/**
 * \ingroup etherdriver
 *
 * EthernetデバイスからEthernetフレームを読み込む.  read() 関数から
 * 呼び出す必要がある。
 *
 * この関数は実際にフレームが受信されるまでブロックする。タイムアウトはしない。
 *
 * @param devptr
 *      このethernetデバイス用のXinuのデバイステーブルのエントリへのポインタ
 * @param buf
 *      Ethernetフレームを受信するバッファ。受信されるフレームは宛先のMAC
 *      アドレスから始まり、ペイロードで終わる。
 * @param len
 *      受信するEthernetフレームの（バイト単位の）最大長（ @p buf のサイズ）
 *
 * @return
 *      Ethernetデバイスが現在upでない場合は ::SYSERR; そうでない場合は、
 *      受信して @p buf に書き出したEthernetフレームの実際の長さ
 */
devcall etherRead(device *devptr, void *buf, uint len);

/**
 * \ingroup etherdriver
 *
 * Ethernetフレーム（一部フィールドは除く）をEthernetデバイスに書き出す.
 * write() 関数から呼び出す必要がある。
 *
 * この関数は実際には後ほど送信するフレームをバッファリングしているだけで
 * ある。したがって、この関数が復帰したときにフレームが実際に送信されている
 * 保証はない。
 *
 * @param devptr
 *      このethernetデバイス用のXinuのデバイステーブルのエントリへのポインタ
 * @param buf
 *      送信するEthernetフレームが格納されているバッファ。宛先のMACアドレスで
 *      始まり、ペイロードで終わっていなければならない。
 * @param len
 *      送信するEthernetフレームの（バイト単位の）長さ
 *
 * @return
 *      パケットが短すぎる、長すぎる、または、Ethernetデバイスが現在upされて
 *      いない場合は ::SYSERR; そうでない場合は、後ほど書き出されて送信される
 *      バイト数である @p len
 */
devcall etherWrite(device *devptr, const void *buf, uint len);

/**
 * \ingroup etherdriver
 *
 * Ethernetデバイス上で制御関数を実行する.  control() 関数から呼び出す
 * 必要がある。
 *
 * @param devptr
 *      このethernetデバイス用のXinuのデバイステーブルのエントリへのポインタ
 * @param req
 *      実行する制御リクエスト
 * @param arg1
 *      （もしあれば）制御リクエスに渡す第1引数
 * @param arg2
 *      （もしあれば）制御リクエスに渡す第2引数
 *
 * @return
 *      制御リクエストの結果、または、制御リクエスト @p req が認識できなかった
 *      場合は ::SYSERR
 */
devcall etherControl(device *devptr, int req, long arg1, long arg2);

/**
 * \ingroup etherdriver
 *
 * Ethernetデバイスに関する情報を表示する.
 *
 * @param minor
 *      情報を表示するEthernetデバイスのマイナー番号
 */
void etherStat(ushort minor);

/**
 * \ingroup etherdriver
 *
 * Ethernetデバイスに関するスループットデータを表示する.
 *
 * @param minor
 *      スループットデータを表示するEthernetデバイスのマイナー番号
 */
void etherThroughput(ushort minor);

interrupt etherInterrupt(void);

int colon2mac(char *, unsigned char *);
int allocRxBuffer(struct ether *, int);
int waitOnBit(volatile uint *, uint, const int, int);

#endif                          /* _ETHER_H_ */
