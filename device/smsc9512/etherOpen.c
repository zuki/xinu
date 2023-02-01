/**
 * @file etherOpen.c
 *
 * SMSC LAN9512 USB Ethernetアダプタデバイスをオープンするためのコード.
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include "smsc9512.h"
#include <bufpool.h>
#include <ether.h>
#include <stdlib.h>
#include <string.h>
#include <usb_core_driver.h>

/* smsc9512用の etherOpen() の実装; この関数のドキュメントは
 * ether.h を参照  */
/**
 * @details
 *
 * SMSC LAN9512固有の注記:  USBの動的デバイスモデルを同時にXinuの静的
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

    /* USBデバイスが実際に接続されるのを待つ  */
    if (smsc9512_wait_device_attached(devptr->minor) != USB_STATUS_SUCCESS)
    {
        goto out_restore;
    }

    /* デバイスがdownしている場合は失敗  */
    ethptr = &ethertab[devptr->minor];
    if (ethptr->state != ETH_STATE_DOWN)
    {
        goto out_restore;
    }

    /* Tx転送のためのバッファプールを作成する  */
    ethptr->outPool = bfpalloc(sizeof(struct usb_xfer_request) + ETH_MAX_PKT_LEN +
                                   SMSC9512_TX_OVERHEAD,
                               SMSC9512_MAX_TX_REQUESTS);
    if (ethptr->outPool == SYSERR)
    {
        goto out_restore;
    }

    /* Rxパケットのためのバッファプールを作成する（実際のUSB転送用ではない。
     * それは別に割り当てられる） */
    ethptr->inPool = bfpalloc(sizeof(struct ethPktBuffer) + ETH_MAX_PKT_LEN,
                              ETH_IBLEN);
    if (ethptr->inPool == SYSERR)
    {
        goto out_free_out_pool;
    }

    /* csrフィールドを悪用してUSBデバイス構造体へのポインタを保存する。
     * 少なくともそれはほぼ同等である。なぜなら、実際にデバイスハード
     * ウェアと通信するために必要なものだからである。
     */
    udev = ethptr->csr;

    /* MACアドレスをセットする */
    if (smsc9512_set_mac_address(udev, ethptr->devAddress) != USB_STATUS_SUCCESS)
    {
        goto out_free_in_pool;
    }

    /* Txリクエストを初期化する  */
    {
        struct usb_xfer_request *reqs[SMSC9512_MAX_TX_REQUESTS];
        for (i = 0; i < SMSC9512_MAX_TX_REQUESTS; i++)
        {
            struct usb_xfer_request *req;

            req = bufget(ethptr->outPool);
            usb_init_xfer_request(req);
            req->dev = udev;
            /* smsc9512_bind_device() でチェックされたTxエンドポイントを
             * 割り当てる */
            req->endpoint_desc = udev->endpoints[0][1];
            req->sendbuf = (uint8_t*)req + sizeof(struct usb_xfer_request);
            req->completion_cb_func = smsc9512_tx_complete;
            req->private = ethptr;
            reqs[i] = req;
        }
        for (i = 0; i < SMSC9512_MAX_TX_REQUESTS; i++)
        {
            buffree(reqs[i]);
        }
    }

    /* Rxリクエストを割り当て発行する。TODO: これは開放されていない */
    for (i = 0; i < SMSC9512_MAX_RX_REQUESTS; i++)
    {
        struct usb_xfer_request *req;

        req = usb_alloc_xfer_request(SMSC9512_DEFAULT_HS_BURST_CAP_SIZE);
        if (req == NULL)
        {
            goto out_free_in_pool;
        }
        req->dev = udev;
        /* smsc9512_bind_device() でチェックされたRxエンドポイントを割り当てる */
        req->endpoint_desc = udev->endpoints[0][0];
        req->completion_cb_func = smsc9512_rx_complete;
        req->private = ethptr;
        usb_submit_xfer_request(req);
    }

    /* 実際のハードウェア上で送受信を有効にする。これを行い、割り込みを
     * 復元した後、Rx転送は着信パケットの受信をいつでも完了することが
     * できる
     */
    udev->last_error = USB_STATUS_SUCCESS;
    smsc9512_set_reg_bits(udev, MAC_CR, MAC_CR_TXEN | MAC_CR_RXEN);
    smsc9512_write_reg(udev, TX_CFG, TX_CFG_ON);
    if (udev->last_error != USB_STATUS_SUCCESS)
    {
        goto out_free_in_pool;
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
    return retval;
}
