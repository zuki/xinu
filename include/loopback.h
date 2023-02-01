/**
 * @file loopback.h
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _LOOPBACK_H_
#define _LOOPBACK_H_

#include <device.h>
#include <stddef.h>
#include <semaphore.h>

#ifndef LOOP_BUFFER
#define LOOP_BUFFER 1024        /**< ループバックバッファ長            */
#endif

/* ループバックデバイスの状態定義 */
#define LOOP_STATE_FREE     0
#define LOOP_STATE_ALLOC    1

/* loopbackControl() 関数  */
#define LOOP_CTRL_SET_FLAG  0x01 /**< フラグをセットする                */
#define LOOP_CTRL_CLR_FLAG  0x02 /**< フラグをクリアする                */

/* ループバックフラグ */
#define LOOP_NONBLOCK       0x01 /**< 読み書きでブロックしない          */

/**
 * ループバックデバイスコントロールブロック
 */
struct loopback
{
    int state;                  /**< 状態: LOOP_STATE_*                 */
    int index;                  /**< バッファ内の最初の文字のインデックス */
    int flags;                  /**< ループバック制御フラグ             */
    semaphore sem;              /**< バッファ荷の文字数                 */
    uchar buffer[LOOP_BUFFER];  /**< 入力バッファ                       */
};

extern struct loopback looptab[];

/* ドライバ関数 */
devcall loopbackInit(device *);
devcall loopbackOpen(device *);
devcall loopbackClose(device *);
devcall loopbackRead(device *, void *, uint);
devcall loopbackWrite(device *, const void *, uint);
devcall loopbackGetc(device *);
devcall loopbackPutc(device *, char);
devcall loopbackControl(device *, int, long, long);

#endif                          /* _LOOPBACK_H_ */
