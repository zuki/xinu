/**
 * @file tftp.h
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#ifndef _TFTP_H_
#define _TFTP_H_

#include <stddef.h>
#include <network.h>
#include <stdint.h>

/** @ingroup tftp
 * @def TFTP_OPCODE_RRQ
 * @brief TFTPオプコード: 読み込み要求 */
#define TFTP_OPCODE_RRQ   1
/** @ingroup tftp
 * @def TFTP_OPCODE_WRQ
 * @brief TFTPオプコード: 書き込み要求 */
#define TFTP_OPCODE_WRQ   2
/** @ingroup tftp
 * @def TFTP_OPCODE_DATA
 * @brief TFTPオプコード: データ送信 */
#define TFTP_OPCODE_DATA  3
/** @ingroup tftp
 * @def TFTP_OPCODE_ACK
 * @brief TFTPオプコード: 確認応答 */
#define TFTP_OPCODE_ACK   4
/** @ingroup tftp
 * @def TFTP_OPCODE_ERROR
 * @brief TFTPオプコード: エラー */
#define TFTP_OPCODE_ERROR 5

/** @ingroup tftp
 * @def TFTP_RECV_THR_STK
 * @brief TFTP受信スレッドスタックサイズ */
#define TFTP_RECV_THR_STK   NET_THR_STK
/** @ingroup tftp
 * @def TFTP_RECV_THR_PRIO
 * @brief TFTP受信スレッド優先度 */
#define TFTP_RECV_THR_PRIO  NET_THR_PRIO

/** @ingroup tftp
 * @def TFTP_BLOCK_TIMEOUT
 * @brief TFTP転送を中止する前に、最初のブロック以外のブロックを待機させる最大秒数 */
#define TFTP_BLOCK_TIMEOUT      10

/** @ingroup tftp
 * @def TFTP_INIT_BLOCK_TIMEOUT
 * @brief RREQを再送する前に最初のブロックを待機させる最大秒数。 */
#define TFTP_INIT_BLOCK_TIMEOUT 1

/** @ingroup tftp
 * @def TFTP_INIT_BLOCK_MAX_RETRIES
 * @brief 最初のRREQを送信する最大回数。 */
#define TFTP_INIT_BLOCK_MAX_RETRIES 10

/** @ingroup tftp
 * @def TFTP_BLOCK_SIZE
 * @brief TFTPブロックサイズ */
#define TFTP_BLOCK_SIZE     512

//#define ENABLE_TFTP_TRACE

#ifdef ENABLE_TFTP_TRACE
#  include <stdio.h>
#  include <thread.h>
#  define TFTP_TRACE(format, ...)                                     \
do                                                                    \
{                                                                     \
    fprintf(stderr, "%s:%d (%d) ", __FILE__, __LINE__, gettid());     \
    fprintf(stderr, format, ## __VA_ARGS__);                          \
    fprintf(stderr, "\n");                                            \
} while (0)
#else
#  define TFTP_TRACE(format, ...)
#endif

/**
 * @ingroup tftp
 * TFTPパケット構造体
*/
struct tftpPkt
{
    uint16_t opcode;        /**< オプコード */
    union
    {
        struct
        {
            char filename_and_mode[2 + TFTP_BLOCK_SIZE];    /**< ファイル名とモード */
        } RRQ;
        struct
        {
            uint16_t block_number;          /**< ブロック番号 */
            uint8_t data[TFTP_BLOCK_SIZE];  /**< データ */
        } DATA;
        struct
        {
            uint16_t block_number;          /**< ブロック番号 */
        } ACK;
    };
};

/** @ingroup tftp
 * @def TFTP_MAX_PACKET_LEN
 * @brief TFTP最大パケットサイズ */
#define TFTP_MAX_PACKET_LEN      516

/**
 * @ingroup tftp
 *
 * @var tftpRecvDataFunc
 *
 * tftpGet()でダウンロードしたデータを消費する、呼び出し元が提供する
 * コールバック関数の型。 詳細は tftpGet() を参照。
 */
typedef int (*tftpRecvDataFunc)(const uchar *data, uint len, void *ctx);

syscall tftpGet(const char *filename, const struct netaddr *local_ip,
                const struct netaddr *server_ip, tftpRecvDataFunc recvDataFunc,
                void *recvDataCtx);

syscall tftpGetIntoBuffer(const char *filename, const struct netaddr *local_ip,
                          const struct netaddr *server_ip, uint *len_ret);

thread tftpRecvPackets(int udpdev, struct tftpPkt *pkt, tid_typ parent);

syscall tftpSendACK(int udpdev, ushort block_number);

syscall tftpSendRRQ(int udpdev, const char *filename);

#endif /* _TFTP_H_ */
