/**
 * @file uartInterrupt.c
 */

/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <uart.h>
#include <thread.h>
#include "pl011.h"

/**
 * @ingroup uarthardware
 *
 * PL011 UARTからの割り込みリクエストを処理する.
 */
interrupt uartInterrupt(void)
{
    uint u;

    /* この割り込みハンドラが終了する前に他のスレッドがスケジュールされる
     * のを防ぐために resdefer を設定する。これによりこの割込みハンドラが
     * リエントラントに実行されることを防ぐ */
    extern int resdefer;
    resdefer = 1;

    /* すべてのUARTについて割り込みをチェックする。注: 'uarttab' に登録されて
     * いるすべてのUARTがPL011 UARTであると仮定している */
    for (u = 0; u < NUART; u++)
    {
        uint mis, count;
        uchar c;
        volatile struct pl011_uart_csreg *regptr;
        struct uart *uartptr;

        /* Get a pointer to the UART structure and a pointer to the UART's
         * hardware registers.  */
        uartptr = &uarttab[u];
        regptr = uartptr->csr;

        /* "Masked Interrupt Status" レジスタをチェックしてどの割り込みが
         * 発生したのか調べ、その割り込みを処理する  */
        mis = regptr->mis;
        if (mis & PL011_MIS_TXMIS)
        {
            /* 送信割り込みがアサートされた。FIFOが有効な場合、これは送信FIFOの
             * データ量がプログラムされたトリガレベル以下になったときに発生する。
             * FIFOが無効な場合、"Tx Holding"レジスタが空の場合に発生する。
             */
            /* このUARTで受信した送信割り込みの数を増分する。*/
            uartptr->oirq++;

            /* 送信割り込みを明示的にクリアする。これは、送信割り込みトリガ
             * レベルを満たすだけのFIFOバイト数が出力バッファにない場合が
             * あるので必要である。FIFOが無効の場合、送信するバイトが0であり
             * "Tx Holidng"レジスタを埋めるものがない場合に適用される。
             */
            regptr->icr = PL011_ICR_TXIC;

            /* 出力バッファに保留中のバイトがある場合、残バイトがなくなるか、
             * 送信FIFOに空き容量がなくなるまでUARTに書き込む。(FIFOが無効な
             * 場合、"Tx Holidng"レジスタはサイズ1のFIFOのように動作するため、
             * このコードはそのまま動作する)。保留バイトがない場合はUARTは
             * アイドル状態であるので "oidle" フラグをセットし、次の
             * uartWrite() の呼び出しでハードウェアに直接1バイトを書き出す
             * ことで送信を再開できるようにする。
             */
            if (uartptr->ocount > 0)
            {
                count = 0;
                do
                {
                    regptr->dr = uartptr->out[uartptr->ostart];
                    uartptr->ostart = (uartptr->ostart + 1) % UART_OBLEN;
                    uartptr->ocount--;
                    count++;
                } while (!(regptr->fr & PL011_FR_TXFF) && (uartptr->ocount > 0));

                /* 1バイト以上が出力バッファから正常に削除され、UARTハードウェアに
                 * 書き出されたので、このUARTに書き込まれたバイトの合計数を増分し、
                 * uartWrite() で待機しているスレッドに @count 回、出力バッファに
                 * 空きスペースができたことを通知する。
                 */
                uartptr->cout += count;
                signaln(uartptr->osema, count);
            }
            else
            {
                uartptr->oidle = TRUE;
            }
        }
        if (mis & PL011_MIS_RXMIS)
        {
            /* 受信割り込みがアサートされた。FIFOが有効な場合、これは受信FIFOの
             * データ量がプログラムされたトリガレベル以上になったときに発生する。
             * FIFOが無効な場合、"Rx Holding"レジスタに1バイト入った場合に発生する。
             */

            /* このUARTで受信した受信割り込みの数を増分する */
            uartptr->iirq++;

            /* これまでにバッファリングに成功したバイト数。  */
            count = 0;

            /* 受信FIFOが再び空になるまで受信FIFOからバイトを読み込む（FIFOが
             * 無効の場合、"Rx Holidng"レジスタはサイズ1のFIFOとして動作するため、
             * コードはそのまま動作する）
             */
            do
            {
                /* UART受信FIFOから1バイト取得する */
                c = regptr->dr;
                if (uartptr->icount < UART_IBLEN)
                {
                    /* 入力バッファにこのバイト用のスペースがあるので、それを追加して
                     * 受信した1文字を集計する */
                    uartptr->in[(uartptr->istart +
                                 uartptr->icount) % UART_IBLEN] = c;
                    uartptr->icount++;
                    count++;
                }
                else
                {
                    /* 入力バッファにこのバイト用のスペースが *ない* ので、
                     * それを無視して、overrun回数を集計する */
                    uartptr->ovrrn++;
                }
            } while (!(regptr->fr & PL011_FR_RXFE));

            /* 受信FIFOが空になるまでバイトを読み込んだので、受信割り込みは
             * 自動的にクリアされたことになる。
             */

            /* バッファリングに成功したバイト数だけcinを増分し、
             * 現在 uartRead() でバッファリングデータが利用可能になるのを
             * 待っているスレッド数だけシグナルを送る
             */
            uartptr->cin += count;
            signaln(uartptr->isema, count);
        }
    }

    /* これでUART割り込みハンドラは終了ので、シグナルを受けたスレッドを安全に
     * 起床させることができる
     */
    if (--resdefer > 0)
    {
        resdefer = 0;
        resched();
    }
}
