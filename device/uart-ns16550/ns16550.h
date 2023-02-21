/**
 * @file ns16550.h
 *
 * @brief miniUART
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _NS16550_H_
#define _NS16550_H_

#include <stddef.h>
#include <conf.h>

/**
 * 16550 UART用のコントロール・テータスレジスタ。 この構造体は
 * CSRのベースアドレスに直接マップされる。
 */
struct ns16550_uart_csreg
{
#if UART_CSR_SPACED
    volatile ulong buffer;      /**< receive buffer (read only)         */
                                /**<  OR transmit hold (write only)     */
    volatile ulong ier;         /**< interrupt enable                   */
    volatile ulong iir;         /**< interrupt ident (read only)        */
                                /**<  OR FIFO control (write only)      */
    volatile ulong lcr;         /**< line control                       */
    volatile ulong mcr;         /**< modem control                      */
    volatile ulong lsr;         /**< line status                        */
    volatile ulong msr;         /**< modem status                       */
    volatile ulong scr;         /**< scratch                            */
#else

    volatile uchar buffer;      /**< 受信バッファ (RO)                  */
                                /**<  または、送信データ格納 (WO)       */
    volatile uchar ier;         /**< 割り込みイネーブル                 */
    volatile uchar iir;         /**< 割り込み識別                  (RO) */
                                /**<  または、FIFO制御 (RO)             */
    volatile uchar lcr;         /**< ラインコントロール                 */
    volatile uchar mcr;         /**< モデムコントロール                 */
    volatile uchar lsr;         /**< ラインステータス                   */
    volatile uchar msr;         /**< モデムステータス                   */
    volatile uchar scr;         /**< スクラッチ                         */
#endif
};

/* コントロール・ステータスレジスタの別名                               */
#define rbr buffer              /**< 受信バッファ (RO)                  */
#define thr buffer              /**< 送信データ格納 (WO)                */
#define fcr iir                 /**< FIFO制御 (WO)                      */
#define dll buffer              /**< 除数ラッチ低位バイト               */
#define dlm ier                 /**< 除数ラッチ高位バイト               */

#define UART_FIFO_LEN  8

/* コントロール・ステータスレジスタのUARTビットフラグ                   */
/* 割り込みイネーブルビット                                             */
#define UART_IER_ERBFI  0x01    /**< 受信データ割り込み                 */
#define UART_IER_ETBEI  0x02    /**< 送信バッファエンプティ割り込み     */
#define UART_IER_ELSI   0x04    /**< Recv line status (miniUARTはN/A） */
#define UART_IER_EMSI   0x08    /**< Modem status (miniUARTはN/A）     */

/* 割り込み識別マスク */
#define UART_IIR_IRQ    0x01    /**< 割り込み保留ビット                 */
#define UART_IIR_IDMASK 0x0E    /**< 割り込みID用の3ビットフィールド    */
#define UART_IIR_MSC    0x00    /**< モデムステータス変更 (miniUARTはN/A) */
#define UART_IIR_THRE   0x02    /**< 送信データ格納レジスタエンプティ   */
#define UART_IIR_RDA    0x04    /**< 受信データあり                     */
#define UART_IIR_RLSI   0x06    /**< レシーバのラインステータス割り込み  (miniUARTはN/A)*/
#define UART_IIR_RTO    0x0C    /**< レシーバタイムアウト (miniUARTはN/A)               */

/* FIFO制御ビット （UAR_IIR Write only) */
#define UART_FCR_EFIFO  0x01    /**< ハードウェアFIFO in/outを有効化    */
#define UART_FCR_RRESET 0x02    /**< 受信FIOをリセット                  */
#define UART_FCR_TRESET 0x04    /**< 送信FIFOをリセット                 */
#define UART_FCR_TRIG0  0x00    /**< RCVR FIFO trigger level one char   */
#define UART_FCR_TRIG1  0x40    /**< RCVR FIFO trigger level 1/4        */
#define UART_FCR_TRIG2  0x80    /**< RCVR FIFO trigger level 2/4        */
#define UART_FCR_TRIG3  0xC0    /**< RCVR FIFO trigger level 3/4        */


/* ラインコントロールビット */
#define UART_LCR_DLAB   0x80    /**< 除数ラッチアクセスビット           */
#define UART_LCR_8N1    0x03    /**< 8 bits, no parity, 1 stop          */

/* モデムコントロールビット */
#define UART_MCR_OUT2   0x08    /**< ユーザ定義のOUT2.                  */
#define UART_MCR_LOOP   0x10    /**< ループバックテストモードを有効化   */

/* ラインステータスビット */
#define UART_LSR_DR     0x01    /**< 受信データあり                     */
#define UART_LSR_THRE   0x20    /**< 送信データ格納レジスタエンプティ   */
#define UART_LSR_TEMT   0x40    /**< トランスミッタエンプティ           */

#endif                          /* _NS16550_H_ */
