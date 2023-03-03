/**
 * @file etherControl.c
 *
 * Simple contoller to handle requests of the MicroChip LAN7800 device.
 *
 * Authors: Patrick J. McGee
 *          Rade Latinovich
 */
/* Embedded Xinu, Copyright (C) 2008, 2013.  All rights reserved. */

#include <ether.h>
#include <network.h>
#include <string.h>
#include "lan7800.h"

/**
 * @ingroup lan7800
 *
 * @brief lan7800用のetherControl() の実装. ether.hにあるこの関数の文書を参照
 *
 * @param devptr
 *      このethernetデバイス用のXinuのデバイステーブルのエントリへのポインタ
 * @param req
 *      実行する制御リクエスト
 * @param arg1
 *      （もしあれば）制御リクエスに渡す第1引数
 * @param arg2
 *      （もしあれば）制御リクエスに渡す第2引数
 *
 * @return
 *      制御リクエストの結果、または、制御リクエスト @p req が認識できなかった
 *      場合は ::SYSERR
 */
devcall etherControl(device *devptr, int req, long arg1, long arg2)
{
    struct usb_device *udev;
    usb_status_t status;
    struct netaddr *addr;
    struct ether *ethptr;
    uint32_t buf;

    ethptr = &ethertab[devptr->minor];
    udev = ethptr->csr;
    if (udev == NULL)
    {
        return SYSERR;
    }

    status = USB_STATUS_SUCCESS;

    switch (req)
    {
    /* MACアドレスをデバイスに書き込む */
    case ETH_CTRL_SET_MAC:
        status = lan7800_set_mac_address(udev, (const uchar*)arg1);
        break;

    /* デバイスからMACアドレスを取得する */
    case ETH_CTRL_GET_MAC:
        status = lan7800_get_mac_address(udev, (uchar*)arg1);
        break;

    /* ループバックモードを有効/無効にする */
    case ETH_CTRL_SET_LOOPBK:

        // disable tx and rx
        lan7800_read_reg(udev, LAN7800_MAC_RX, &buf);
        buf &= ~(LAN7800_MAC_RX_RXEN_);
        lan7800_write_reg(udev, LAN7800_MAC_RX, buf);

        lan7800_read_reg(udev, LAN7800_MAC_TX, &buf);
        buf &= ~(LAN7800_MAC_TX_TXEN_);
        lan7800_write_reg(udev, LAN7800_MAC_TX, buf);

        status = lan7800_modify_reg(udev, LAN7800_MAC_CR, ~LAN7800_MAC_CR_LOOPBACK_,
                    ((bool)arg1 == TRUE) ? LAN7800_MAC_CR_LOOPBACK_ : 0);
        // enable tx and rx
        lan7800_read_reg(udev, LAN7800_MAC_RX, &buf);
        buf |= (LAN7800_MAC_RX_RXEN_);
        lan7800_write_reg(udev, LAN7800_MAC_RX, buf);

        lan7800_read_reg(udev, LAN7800_MAC_TX, &buf);
        buf |= (LAN7800_MAC_TX_TXEN_);
        lan7800_write_reg(udev, LAN7800_MAC_TX, buf);

        break;

    /* リンクヘッダー長を取得する */
    case NET_GET_LINKHDRLEN:
        return ETH_HDR_LEN;

    /* MTUを取得する */
    case NET_GET_MTU:
        return ETH_MTU;

    /* ハードウェアアドレスを取得する  */
    case NET_GET_HWADDR:
        addr = (struct netaddr *)arg1;
        addr->type = NETADDR_ETHERNET;
        addr->len = ETH_ADDR_LEN;
        return etherControl(devptr, ETH_CTRL_GET_MAC, (long)addr->addr, 0);

    /* ブロードキャストアドレスを取得する */
    case NET_GET_HWBRC:
        addr = (struct netaddr *)arg1;
        addr->type = NETADDR_ETHERNET;
        addr->len = ETH_ADDR_LEN;
        memset(addr->addr, 0xFF, ETH_ADDR_LEN);
        break;

    default:
        return SYSERR;
    }

    if (status != USB_STATUS_SUCCESS)
    {
        return SYSERR;
    }

    return OK;
}
