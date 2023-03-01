/**
 * @file uart.h
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _UART_H_
#define _UART_H_

#include <device.h>
#include <semaphore.h>
#include <stddef.h>

/* UARTバッファ長 */
#ifndef UART_IBLEN
#define UART_IBLEN      1024
#endif
#ifndef UART_OBLEN
#define UART_OBLEN      1024
#endif

#define UART_BAUD       115200  /**< コンソールのデフォルト通信速度 */

/**
 * UART制御ブロック
 */
struct uart
{
    /* 関連する構造体へのポインタ */
    void *csr;                  /**< 制御およびステータスレジスタ       */
    device *dev;                /**< デバイス構造体                     */

    /* 統計カウント */
    uint cout;                  /**< 出力文字数                         */
    uint cin;                   /**< ミュウ力文字数                     */
    uint lserr;                 /**< 受信エラー回数                     */
    uint ovrrn;                 /**< オーバーラン文字数                 */
    uint iirq;                  /**< 入力IRQ回数                        */
    uint oirq;                  /**< 出力IRQ回数                        */

    /* UART入力フィールド */
    uchar iflags;               /**< 入力フラグ                         */
    semaphore isema;            /**< 入力redayバイト数                  */
    ushort istart;              /**< 最初のバイトのインデックス         */
    ushort icount;              /**< バッファにあるバイト数             */
    uchar in[UART_IBLEN];       /**< インプットバッファ                 */

    /* UART出力フィールド */
    uchar oflags;               /**< 出力フラグ                         */
    semaphore osema;            /**< バッファの空きスペース数           */
    ushort ostart;              /**< 最初のバイトのインデックス         */
    ushort ocount;              /**< バッファにあるバイト数             */
    uchar out[UART_OBLEN];      /**< 出力バッファ                       */
    volatile bool oidle;        /**< UART送信器はアイドル状態？         */
    mutex_t olock;		        /**< counterを守るmutex                 */
};

extern struct uart uarttab[];

/* UART入力フラグ */
#define UART_IFLAG_NOBLOCK    0x0001 /**< ノンブロッキング入力を行う    */
#define UART_IFLAG_ECHO       0x0002 /**< 入力をエコー                  */

/* UART出力フラグ */
#define UART_OFLAG_NOBLOCK    0x0001 /**< ノンブロッキング出力を行う    */

/* uartControl() 関数  */
#define UART_CTRL_SET_IFLAG   0x0010 /**< 入力フラグをセット            */
#define UART_CTRL_CLR_IFLAG   0x0011 /**< 入力フラグをクリア            */
#define UART_CTRL_GET_IFLAG   0x0012 /**< 入力フラグを取得              */
#define UART_CTRL_SET_OFLAG   0x0013 /**< 出力フラグをセット            */
#define UART_CTRL_CLR_OFLAG   0x0014 /**< 出力フラグをクリア            */
#define UART_CTRL_GET_OFLAG   0x0015 /**< 出力フラグを取得              */
#define UART_CTRL_OUTPUT_IDLE 0x0016 /**< 送信器はアイドル状態か          */

/* ドライバ関数 */
devcall uartInit(device *);
devcall uartRead(device *, void *, uint);
devcall uartWrite(device *, const void *, uint);
devcall uartGetc(device *);
devcall uartPutc(device *, char);
devcall uartControl(device *, int, long, long);
interrupt uartInterrupt(void);
void uartStat(ushort);

/**
 * @ingroup uarthardware
 *
 * UARTハードウェアを初期化する.
 *
 * @param devptr デバイスへのポインタ
 */
devcall uartHwInit(device *);

/**
 * @ingroup uarthardware
 *
 * 1文字をUARTに直ちにputする.
 */
void uartHwPutc(void *, uchar);

/**
 * @ingroup uarthardware
 *
 * UARTに関するハードウェア固有の統計を表示する.
 */
void uartHwStat(void *);

#endif                          /* _UART_H_ */
