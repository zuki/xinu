/**
 * @file etherWrite.c
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include "smsc9512.h"
#include <bufpool.h>
#include <ether.h>
#include <interrupt.h>
#include <string.h>
#include <usb_core_driver.h>

/* smsc LAN9512用の etherWrite() の実装; この関数のドキュメントはether.h
 * を参照 */
devcall etherWrite(device *devptr, const void *buf, uint len)
{
    struct ether *ethptr;
    struct usb_xfer_request *req;
    uint8_t *sendbuf;
    uint32_t tx_cmd_a, tx_cmd_b;

    ethptr = &ethertab[devptr->minor];
    if (ethptr->state != ETH_STATE_UP ||
        len < ETH_HEADER_LEN || len > ETH_HDR_LEN + ETH_MTU)
    {
        return SYSERR;
    }

    /* パケット用のバッファを取得する（ブロックされるかもしれない） */
    req = bufget(ethptr->outPool);

    /* パケットデータをバッファにコピーする。デバイス固有のフラグ用に
     * 先頭に2ワード分あける。この2つのフィールドは必須であるが、
     * 基本的には1つのパケットを送信することをハードウェアに伝える
     * ためだけに使用するものであり余分な装飾はない。
     */
    sendbuf = req->sendbuf;
    tx_cmd_a = len | TX_CMD_A_FIRST_SEG | TX_CMD_A_LAST_SEG;
    sendbuf[0] = (tx_cmd_a >> 0)  & 0xff;
    sendbuf[1] = (tx_cmd_a >> 8)  & 0xff;
    sendbuf[2] = (tx_cmd_a >> 16) & 0xff;
    sendbuf[3] = (tx_cmd_a >> 24) & 0xff;
    tx_cmd_b = len;
    sendbuf[4] = (tx_cmd_b >> 0)  & 0xff;
    sendbuf[5] = (tx_cmd_b >> 8)  & 0xff;
    sendbuf[6] = (tx_cmd_b >> 16) & 0xff;
    sendbuf[7] = (tx_cmd_b >> 24) & 0xff;
    STATIC_ASSERT(SMSC9512_LAN7800_TX_OVERHEAD == 8);
    memcpy(sendbuf + SMSC9512_LAN7800_TX_OVERHEAD, buf, len);

    /* USB上で送信するデータの合計サイズをセットする*/
    req->size = len + SMSC9512_LAN7800_TX_OVERHEAD;

    /* データを非同期バルクUSB転送として送信する。言い換えると、USBサブ
     * システムにSMSC LAN9512 USB EthernetアダプタにUSB経由でデータの
     * 送信を開始するよう指示する。その後、USB上ですべてのデータが転送
     * されるとUSBサブシステムから smsc9512_tx_complete() がコールされる
     */
    usb_submit_xfer_request(req);

    /* 書き出したパケットの長さを返す（追加したデバイス固有フィールドは
     * 含まない） */
    return len;
}
