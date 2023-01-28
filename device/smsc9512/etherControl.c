/**
 * @file etherControl.c
 */
/* Embedded Xinu, Copyright (C) 2008, 2013.  All rights reserved. */

#include <ether.h>
#include <network.h>
#include <string.h>
#include "smsc9512.h"

/* smsc9512 etherControl() の実装; ether.h にあるこの関数の文書を参照  */
devcall etherControl(device *devptr, int req, long arg1, long arg2)
{
    struct usb_device *udev;
    usb_status_t status;
    struct netaddr *addr;
    struct ether *ethptr;

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
        status = smsc9512_set_mac_address(udev, (const uchar*)arg1);
        break;

    /* デバイスからMACアドレスを取得する */
    case ETH_CTRL_GET_MAC:
        status = smsc9512_get_mac_address(udev, (uchar*)arg1);
        break;

    /* ループバックモードを有効/無効にする */
    case ETH_CTRL_SET_LOOPBK:
        status = smsc9512_modify_reg(udev, MAC_CR, ~MAC_CR_LOOPBK,
                                     ((bool)arg1 == TRUE) ? MAC_CR_LOOPBK : 0);
        break;

    /* リンクヘッダー長を取得する */
    case NET_GET_LINKHDRLEN:
        return ETH_HDR_LEN;

    /* MTUを取得する */
    case NET_GET_MTU:
        return ETH_MTU;

    /* ハードウェアあれですを取得する  */
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
