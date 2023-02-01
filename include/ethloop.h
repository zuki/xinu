/**
 * @file ethloop.h
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _ETHLOOP_H_
#define _ETHLOOP_H_

#include <stddef.h>
#include <device.h>
#include <ethernet.h>
#include <semaphore.h>

#define ELOOP_MTU          1500
#define ELOOP_LINKHDRSIZE  ETH_HDR_LEN

#define ELOOP_BUFSIZE      ELOOP_MTU + ELOOP_LINKHDRSIZE
#define ELOOP_NBUF         100

#define ELOOP_CTRL_GETHOLD	1
#define ELOOP_CTRL_SETFLAG  2
#define ELOOP_CTRL_CLRFLAG	3

#define ELOOP_FLAG_HOLDNXT	0x01  /**< 次に書き込まれたPKTを保留にする  */
#define ELOOP_FLAG_DROPNXT	0x04  /**< 次に書かきもれたPKTを破棄する    */
#define ELOOP_FLAG_DROPALL	0x08  /**< 書き込まれたすべてのPKTを破棄する */

#define ELOOP_STATE_FREE        0
#define ELOOP_STATE_ALLOC       1

/**
 * ループバックデバイスコントロールブロック
 */
struct ethloop
{
    int state;                      /**< デバイスの状態                     */
    device *dev;                    /**< デバイステーブルエントリ           */
    int poolid;                     /**< バッファプールのID                 */
    uchar flags;                    /**< フラグ                             */

    /* パケットキュー */
    int index;                      /**< バッファ内の最初のパケットのindex  */
    semaphore sem;                  /**< バッファ内のパケット数             */
    int count;                      /**< バッファ内のパケット数             */
    char *buffer[ELOOP_NBUF];       /**< 入力バッファ                       */
    int pktlen[ELOOP_NBUF];         /**< バッファ二日パケット長             */

    /* 保持パケット */
    semaphore hsem;                 /**< 保持パケット数                     */
    char *hold;                     /**< 保持バッファ                       */
    int holdlen;                    /**< 保持バッファ内のパケット長         */

    /* 統計 */
    uint nout;                      /**< 書き込まれたパケット数             */
};

extern struct ethloop elooptab[];

/* ドライバ関数 */
devcall ethloopInit(device *);
devcall ethloopOpen(device *);
devcall ethloopClose(device *);
devcall ethloopRead(device *, void *, uint);
devcall ethloopWrite(device *, const void *, uint);
devcall ethloopControl(device *, int, long, long);

#endif                          /* _ETHLOOP_H_ */
