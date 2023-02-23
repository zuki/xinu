/**
 * @file etherOpen.c
 *
 * @ingroup ether_lan7800
 *
 * LAN7800 USB Ethernetアダプタデバイスをオープンするためのコード.
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <bufpool.h>
#include <ether.h>
#include <stdlib.h>
#include <string.h>
#include <clock.h>
#include <platform.h>
#include <usb_core_driver.h>
#include "lan7800.h"
#include <stddef.h>

extern syscall kprintf(const char *fmt, ...);

/* LAN7800用の etherOpen() の実装; この関数のドキュメントは
 * ether.h を参照  */
/**
 * @ingroup ether_lan7800
 *
 * @details
 *
 * LAN7800固有の注記:  USBの動的デバイスモデルを同時にXinuの静的
 * デバイスモデルと使用するための回避策として、この関数は対応するUSB
 * デバイスが実際にUSBに接続されるまでブロックされる。厳密にいえば、
 * デバイスが取り外せないものであっても、これが実際に発生するかは保証
 * されない。
 */
devcall etherOpen(device *devptr)
{
    struct ether *ethptr;
    struct usb_device *udev;
    irqmask im;
    uint i;
    int retval = SYSERR;

    im = disable();
    kprintf("[etherOpen]: attached fd=%d\r\n", devptr->minor);
    /* USBデバイスが実際に接続されるのを待つ  */
    if (lan7800_wait_device_attached(devptr->minor) != USB_STATUS_SUCCESS)
    {
        goto out_restore;
    }
    kprintf("etherOpen: 3\r\n");
    /* デバイスがdownしていない場合は失敗  */
    ethptr = &ethertab[devptr->minor];
    if (ethptr->state != ETH_STATE_DOWN)
    {
        goto out_restore;
    }
    kprintf("etherOpen: 4\r\n");
    /* Tx転送用のバッファプールを作成する  */
    ethptr->outPool = bfpalloc(sizeof(struct usb_xfer_request) +
                            ETH_MAX_PKT_LEN + TX_OVERHEAD,
                            MAX_TX_REQUESTS);
    if (ethptr->outPool == SYSERR)
    {
        goto out_restore;
    }
    kprintf("etherOpen: 5\r\n");
    /* Rxパケット用のバッファプールを作成する（実際のUSB転送用ではない。
     * それは別に割り当てられる） */
    ethptr->inPool = bfpalloc(sizeof(struct ethPktBuffer) + ETH_MAX_PKT_LEN,
                              ETH_IBLEN);
    if (ethptr->inPool == SYSERR)
    {
        goto out_free_out_pool;
    }
    kprintf("etherOpen: 6\r\n");
    /* csrフィールドを悪用してUSBデバイス構造体へのポインタを保存する。
     * 少なくとも両者はほぼ同等である。なぜなら、実際にデバイスハード
     * ウェアと通信するために必要なものだからである。
     */
    udev = ethptr->csr;

    /* 次に、LAN7800を使用可能な状態にする。これは主にLAN7800のレジスタへの
     * 書き込みである。しかし、これはUSBで接続されたUSBイーサネットアダプタであり、
     * メモリマップドレジスタではない。そのため、USBのコントロール転送を使って
     * レジスタの読み書きを行う。これは少々面倒であり、またメモリアクセスと違って
     * USBコントロール転送は失敗する可能性もある。しかし、ここでは、必要な読み書きを
     * すべて行い、最後にエラーが発生したかどうかをチェックする遅延エラーチェックを
     * 行っている。
     */

    udev->last_error = USB_STATUS_SUCCESS;

    retval = lan7800_init(udev, &ethptr->devAddress[0]);
    if (retval < 0) {
        goto out_free_in_pool;
    }

    kprintf("etherOpen: 7\r\n");

    /* Txリクエストを初期化する  */
    {
        struct usb_xfer_request *reqs[MAX_TX_REQUESTS];
        for (i = 0; i < MAX_TX_REQUESTS; i++)
        {
            struct usb_xfer_request *req;

            req = bufget(ethptr->outPool);
            usb_init_xfer_request(req);
            req->dev = udev;
            /* lan7800_bind_device() でチェックされたTxエンドポイントを
             * 割り当てる */
            req->endpoint_desc = udev->endpoints[0][1];
            req->sendbuf = (uint8_t*)req + sizeof(struct usb_xfer_request);
            req->completion_cb_func = lan7800_tx_complete;
            req->private = ethptr;
            reqs[i] = req;
        }
        for (i = 0; i < MAX_TX_REQUESTS; i++)
        {
            buffree(reqs[i]);
        }
        kprintf("etherOpen: 8\r\n");
    }

    /* Rxリクエストを割り当て発行する。TODO: これは開放されていない */
    for (i = 0; i < MAX_RX_REQUESTS; i++)
    {
        struct usb_xfer_request *req;

        req = usb_alloc_xfer_request(DEFAULT_BURST_CAP_SIZE);
        if (req == NULL)
        {
            goto out_free_in_pool;
        }
        req->dev = udev;
        /* lan7800_bind_device() でチェックされたRxエンドポイントを割り当てる */
        req->endpoint_desc = udev->endpoints[0][0];
        req->completion_cb_func = lan7800_rx_complete;
        req->private = ethptr;
        usb_submit_xfer_request(req);
        kprintf("etherOpen: 9\r\n");
    }

    /* 成功!  デバイスに ETH_STATE_UP をセットする */
    ethptr->state = ETH_STATE_UP;
    retval = OK;
    goto out_restore;

out_free_in_pool:
    bfpfree(ethptr->inPool);
out_free_out_pool:
    bfpfree(ethptr->outPool);
out_restore:
    restore(im);
    kprintf("etherOpen: 10\r\n");
    return retval;
}
