/**
 * @file etherInit.c
 *
 * LAN7800 USB Ethernetアダプタを初期化する
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include "lan7800.h"
#include <clock.h>
#include <ether.h>
#include <memory.h>
#include <platform.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <system/platforms/arm-rpi3/rpi-mailbox.h>
#include <usb_core_driver.h>

extern syscall kprintf(const char *fmt, ...);

/* Ethernetデバイスのグローバルテーブル  */
struct ether ethertab[NETHER];

/**
 * 指定されたEthernetデバイスが実際にまだシステム（ここではUSB）に
 * 接続されているか否かを示すセマフォ。他のドライバが必要であれば
 * <code>struct ::ether</code> に移動させるかもしれない。
 */
static semaphore lan7800_attached[NETHER];

/**
 * 指定されたUSBデバイスへのLAN7800ドライバのバインドを試みる。
 * @ref usb_device_driver::bind_device "bind_device" の
 * LAN7800ドライバ用の実装であり、そのドキュメントに記載されて
 * いる動作に準拠している。
 */
static usb_status_t
lan7800_bind_device(struct usb_device *udev)
{
    struct ether *ethptr;

    /* USBコアが既にメモリに読み込んでいるUSBデバイスの標準デバイス
     * ディスクリプタを確認して、これが実際にLAN7800であることを確認する。
     * また、パケットの送受信に必要なエンドポイントが存在するか、デバイスが
     * High Speedで動作するかも確認する。
     */
    if (udev->descriptor.idVendor != LAN7800_VENDOR_ID ||
        udev->descriptor.idProduct != LAN7800_PRODUCT_ID ||
        udev->interfaces[0]->bNumEndpoints < 2 ||
        (udev->endpoints[0][0]->bmAttributes & 0x3) != USB_TRANSFER_TYPE_BULK ||
        (udev->endpoints[0][1]->bmAttributes & 0x3) != USB_TRANSFER_TYPE_BULK ||
        (udev->endpoints[0][0]->bEndpointAddress >> 7) != USB_DIRECTION_IN ||
        (udev->endpoints[0][1]->bEndpointAddress >> 7) != USB_DIRECTION_OUT ||
        udev->speed != USB_SPEED_HIGH)
    {
        return USB_STATUS_DEVICE_UNSUPPORTED;
    }

    /* このデバイスがすでにLAN7800にバインドされていないか確認する。
     * TODO: このタイプの複数のデバイスを並行してサポートする  */
    ethptr = &ethertab[0];
    STATIC_ASSERT(NETHER == 1);
    if (ethptr->csr != NULL)
    {
        return USB_STATUS_DEVICE_UNSUPPORTED;
    }

    /* この関数の残りの部分ではLAN7800を使用可能な状態にするが
     * 実際にRxとTxを有効にすることはない（これはetherOpen() で行う）。
     * ここでの作業は主にLAN7800のレジスタへの書き込みである。
     * ただし、これはUSBに接続されたUSBイーサネットアダプタなので
     * メモリマップドレジスタではない。レジスタの読み書きはUSBの
     * コントロール転送を利用して行われる。少し面倒であり、メモリ
     * アクセスと違ってUSBコントロール転送は失敗する可能性もある。
     * しかし、ここでは必要な読み書きをすべて行い、最後にエラーが
     * 発生したか否をチェックする遅延エラーチェックを行っている。
     */

    udev->last_error = USB_STATUS_SUCCESS;

    /* LAN7800をレジスタ軽腕リセットする必要はないはずである。
     * USBコードがLAN9512が接続されたUSBポートのリセットを既に実行
     * しているからである
     */

    /* MACアドレスをセットする */
    //lan7800_set_mac_address(udev, ethptr->devAddress);

    /* 1回のUSB転送で複数のEthernetフレームを受信できるようにする。 */
    //lan7800_set_reg_bits(udev, HW_CFG, HW_CFG_MEF_);

    /* USB Rx転送あたりの最大USB（ネットワークではない！）パケットをセットする。
     * HW_CFG_MEFが設定された場合に必要になる */
    //lan7800_write_reg(udev, BURST_CAP, DEFAULT_BURST_CAP_SIZE / HS_USB_PKT_SIZE);

    /* エラーをチェックして復帰する */
    if (udev->last_error != USB_STATUS_SUCCESS)
    {
        return udev->last_error;
    }

    ethptr->csr = udev;
    udev->driver_private = ethptr;
    kprintf("[lan7800_bind_device]: signal idx=%d, sem=%d\r\n", ethptr - ethertab, lan7800_attached[ethptr - ethertab]);
    signal(lan7800_attached[ethptr - ethertab]);
    return USB_STATUS_SUCCESS;
}

/**
 * デタッチされたLAN7800からLAN7800ドライバをアンバインドする。
 * これはLAN7800ドライバの @ref usb_device_driver::unbind_device
 * "unbind_device"の実装であり、ドキュメントに記載されている動作に準ずる。
 */
static void
lan7800_unbind_device(struct usb_device *udev)
{
    struct ether *ethptr = udev->driver_private;

    /* アタッチされていたセマフォを0にリセットする  */
    wait(lan7800_attached[ethptr - ethertab]);

    /* デバイスをクローズする  */
    etherClose(ethptr->dev);
}

/**
 * LAN7800のUSBデバイスドライバの仕様。これはUSBコアに特化したものであり、
 * Xinuの主要なデバイスとドライバの（静的な）モデルとは関係しない。
 */
static const struct usb_device_driver lan7800_driver = {
    .name          = "LAN7800 USB Ethernet Adapter Driver",
    .bind_device   = lan7800_bind_device,
    .unbind_device = lan7800_unbind_device,
};

/**
 * @ingroup ether_lan9512
 *
 * 指定したEthernetデバイスが実際に接続されるまで待つ.
 *
 * USBは動的なバスであるが、Xinuは静的なデバイスを想定している
 * のでこれが必要となる。したがって、静的なイーサネットデバイスを
 * オープンするコードは対応するUSBデバイスが実際に検出・接続される
 * まで待つ必要がある。面白いことに、USBの規格では、デバイスが
 * ボードに物理的にはんだ付けされている場合であっても、実際に
 * これにかかる時間については制約を設けていない。
 *
 * TODO: 失敗を返すまでに少なくともある一定時間待つようにする
 *
 * @param minor
 *     接続を待つEthernetデバイスのマイナー番号
 *
 * @return
 *      現在のところ ::USB_STATUS_SUCCESS.  TODO: タイムアウトを実装して、
 *      タイムアウトの場合は USB_STATUS_TIMEOUT を返すようにする
 */
usb_status_t
lan7800_wait_device_attached(ushort minor)
{
    kprintf("[wait_device_attached]: start minor=%d\r\n", minor);
    kprintf("[wait_device_attached]: wait sem=%d\r\n", lan7800_attached[minor]);
    wait(lan7800_attached[minor]);
    kprintf("[wait_device_attached]: signal sem=%d\r\n", lan7800_attached[minor]);
    signal(lan7800_attached[minor]);
    return USB_STATUS_SUCCESS;
}

/* smsc9512用の etherInit() の実装; この関数に関するドキュメントは
 * ether.h を参照 */
/**
 * @details
 *
 * LAN7800-固有の注記:  この関数はEthernetドライバがUSBコアに
 * 正常に登録された場合は ::OK、そうでない場合は ::SYSERR を返す。
 * これはUSBの動的デバイスモデルとXinuの静的デバイスモデルを同時に
 * 使用するための回避策であり、この関数が復帰した時に実際にデバイスが
 * 存在する保証はない(存在しない場合、etherOpen() で実際にデバイスを
 * オープンするまで問題は引き伸ばされる)。
 */
devcall etherInit(device *devptr)
{
    struct ether *ethptr;
    usb_status_t status;

    /* このデバイス用の静的な `struct ether' を初期化する */
    ethptr = &ethertab[devptr->minor];
    bzero(ethptr, sizeof(struct ether));
    ethptr->dev = devptr;
    ethptr->state = ETH_STATE_DOWN;
    ethptr->mtu = ETH_MTU;
    ethptr->addressLength = ETH_ADDR_LEN;
    ethptr->isema = semcreate(0);
    if (isbadsem(ethptr->isema))
    {
        goto err;
    }

    lan7800_attached[devptr->minor] = semcreate(0);

    if (isbadsem(lan7800_attached[devptr->minor]))
    {
        goto err_free_isema;
    }

    if (rpi_getmacaddr(ethptr->devAddress) != OK) {
        goto err_free_attached_sema;
    }

    /* このデバイスドライバをUSBコアに登録して復帰する */
    status = usb_register_device_driver(&lan7800_driver);
    if (status != USB_STATUS_SUCCESS)
    {
        goto err_free_attached_sema;
    }
    return OK;

err_free_attached_sema:
    semfree(lan7800_attached[devptr->minor]);
err_free_isema:
    semfree(ethptr->isema);
err:
    return SYSERR;
}
