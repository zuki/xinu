/**
 * @file     etherInterrupt.c
 * @ingroup lan7800
 *
 * このファイルはLAN7800 USB EthernetアダプタのUSB転送完了コールバックを
 * 提供する。これは他のXinu Ethernetドライバに実装されている  etherInterrupt()
 * とほぼ同等であるため、ファイル名がこのようになっているが、USBの仕組み上、
 * 実際の etherInterrupt() 関数は存在しない。LAN7800は単体で割り込みを
 * かけることはできない。実際に起こるのはLAN7800へのUSB送受信が完了した
 * ことによりUSBホストコントローラがCPUに割り込みをかけることである。この
 * 割り込みはUSBホストコントローラドライバによって処理され、USB転送のために
 * 登録されたコールバック関数が呼び出されることになる。
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include "lan7800.h"
#include <bufpool.h>
#include <ether.h>
#include <string.h>
#include <usb_core_driver.h>
#include <kernel.h>

/**
 * @ingroup lan7800
 *
 * Ethernetパケットの送信を目的としたLAN7800 USB Ethernet
 * アダプタのバルクOUTエンドポイントへの非同期USBバルク転送が成功または
 * 失敗したときに割り込みを無効にして実行されるコールバック関数.
 *
 * 現在、この関数がしなければならないことはバッファをプールに返すこと
 * だけである。これにより etherWrite() の中で空きバッファを待っている
 * スレッドを起床させることができる。
 *
 * @param req
 *      完了したUSBバルクOUT転送リクエスト
 */
void lan7800_tx_complete(struct usb_xfer_request *req)
{
    struct ether *ethptr = req->private;

    ethptr->txirq++;
    usb_dev_debug(req->dev, "\n\nLAN7800: Tx complete\n");
    buffree(req);
}

/**
 * @ingroup lan7800
 *
 * 1つ以上のEthernetパケットの受信を目的としたLAN7800 USB Ethernet
 * アダプタのバルクINエンドポイントへの非同期USBバルク転送が成功または
 * 失敗したときに割り込みを無効にして実行されるコールバック関数.
 *
 * この関数は生のUSB転送データをEthernetパケットに分割し、着信パケット
 * キューに格納する（これにより、etherRead() で新しいパケットを待っている
 * スレッドが起床させることができる）。その後、パケットを受信し続けることが
 * できるようにUSBバルク転送リクエストを再送信する必要がある。
 *
 * @param req
 *      完了したUSBバルクIN転送リクエスト
 */
void lan7800_rx_complete(struct usb_xfer_request *req)
{
    struct ether *ethptr = req->private;

    ethptr->rxirq++;
    if (req->status == USB_STATUS_SUCCESS)
    {
        const uint8_t *data, *edata;
        uint32_t rx_cmd_a;
        uint32_t frame_length;

        /* 受信したUSBデータ内の各Ethernetフレームについて実行 */
        for (data = req->recvbuf, edata = req->recvbuf + req->actual_size;
             data + RX_OVERHEAD + ETH_HDR_LEN + ETH_CRC_LEN <= edata;
             data += RX_OVERHEAD + ((frame_length + 3) & ~3))
        {
            /* RxAステータスワードを取得する。これには次のEthernet
             * フレームに関する情報が含まれている */
            rx_cmd_a = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;

            /* frame_lengthを抽出する。これは次のEthernetフレームの長さ
             * （宛先MACアドレスからCRCの終わりとそれに続くペイロードを含む）
             * を指定する（これはRxステータスワードには含まれておらず、
             * RX_OVERHEADから計算する）。
             */
            frame_length = (rx_cmd_a & RX_CMD_A_LEN_MASK_);

            if ((rx_cmd_a & RX_CMD_A_RED_) ||                       // 受信エラー
                (frame_length + RX_OVERHEAD > edata - data) ||      // データ長エラー
                (frame_length > ETH_MAX_PKT_LEN + ETH_CRC_LEN) ||   // パケット長エラー
                (frame_length < ETH_HDR_LEN + ETH_CRC_LEN))         // ヘッダー長エラー
            {
                /* Ethernetアダプタが問題がある、または受信したEthernet
                 * フレームサイズが不正であることを示すエラーフラグを
                 * セットしていた。
                 */
                usb_dev_debug(req->dev, "LAN7800: Tallying rx error "
                              "(rx_cmd_a=0x%08x, frame_length=%u)\n",
                              rx_cmd_a, frame_length);
                ethptr->errors++;
            }
            else if (ethptr->icount == ETH_IBLEN)
            {
                /* バッファには受信パケットを入れるスペースがない */
                usb_dev_debug(req->dev, "LAN7800: Tallying overrun\n");
                ethptr->ovrrun++;
            }
            else
            {
                /* 受信したパケットをバッファに入れる */

                struct ethPktBuffer *pkt;

                pkt = bufget(ethptr->inPool);
                pkt->buf = pkt->data = (uint8_t*)(pkt + 1);
                pkt->length = frame_length - ETH_CRC_LEN;
                memcpy(pkt->buf, data + RX_OVERHEAD, pkt->length);
                ethptr->in[(ethptr->istart + ethptr->icount) % ETH_IBLEN] = pkt;
                ethptr->icount++;

                usb_dev_debug(req->dev, "LAN7800: Receiving "
                              "packet (length=%u, icount=%u)\n",
                              pkt->length, ethptr->icount);

                /* etherRead() で待っているスレッドを起床させる */
                signal(ethptr->isema);
            }
        }
    }
    else
    {
        /* 何らかの理由でUSB転送が失敗した  */
        usb_dev_debug(req->dev, "LAN7800: USB Rx transfer failed\n");
        ethptr->errors++;
    }
    usb_dev_debug(req->dev, "LAN7800: Re-submitting USB Rx request\n");
    usb_submit_xfer_request(req);
}
