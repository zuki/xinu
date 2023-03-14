/**
 * @file etherInit.c
 *
 * LAN7800 USB Ethernetアダプタを初期化する
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <stddef.h>
#include <stdint.h>
#include "lan7800.h"
#include <clock.h>
#include <ether.h>
#include <memory.h>
#include <platform.h>
#include <semaphore.h>
#include <stdlib.h>
#include <usb_core_driver.h>
#include "../../system/platforms/arm-rpi3/bcm2837_mbox.h"
#include <string.h>
#include <kernel.h>
#include "../../system/platforms/arm-rpi3/bcm2837.h"
#include <dma_buf.h>

bool lan7800_isattached = 0;

/* Ethernetデバイスのグローバルテーブル  */
struct ether ethertab[NETHER];

/**
 * 指定されたEthernetデバイスが実際にまだシステム（ここではUSB）に
 * 接続されているか否かを示すセマフォ。他のドライバが必要であれば
 * <code>struct ::ether</code> に移動させるかもしれない。
 */
static semaphore lan7800_attached[NETHER];

/**
 * getEthAddr()を呼びだし後にMACアドレスを格納するグローバル変数.
 * これは後にmemcpy()を使いether構造体のdevAddressメンバにコピーされる。
 */
uchar addr[ETH_ADDR_LEN] = {0};

/**
 * @ingroup lan7800
 *
 * 指定されたUSBデバイスへのLAN7800ドライバのバインドを試みる.
 *
 * @param udev USB構造体へのポインタ
 * @return USBデバイスの状態
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

    udev->last_error = USB_STATUS_SUCCESS;

    /* udevとethptrの相互参照 */
    ethptr->csr = udev;                 // ethptr から udev
    udev->driver_private = ethptr;      // udev から ethptr
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

/* Get static MAC address from Pi 3 B+ chip, based on the XinuPi
 * mailbox technique.
 *
 * @details
 *
 * Get the Pi 3 B+'s MAC address using its ARM->VideoCore (VC) mailbox
 * and assign corresponding values to a global array containing the MAC.
 * This array is then assigned to the devAddress member of the ether structure.
 */
static void
getEthAddr(uint8_t *addr)
{
    /* Initialize the mailbox buffer */
    uint32_t *mailbuffer;
    mailbuffer = dma_buf_alloc(MBOX_BUFLEN / 4);

    /* Fill the mailbox buffer with the MAC address.
     * This function is defined in system/platforms/arm-rpi3/bcm2837_mbox.c */
    get_mac_mailbox(mailbuffer);

    ushort i;
    for (i = 0; i < 2; ++i) {

        /* Access the MAC value within the buffer */
        uint32_t value = mailbuffer[MBOX_HEADER_LENGTH + TAG_HEADER_LENGTH + i];

        /* Store the low MAC values */
        if(i == 0){
            addr[0] = (value >> 0)  & 0xff;
            addr[1] = (value >> 8)  & 0xff;
            addr[2] = (value >> 16) & 0xff;
            addr[3] = (value >> 24) & 0xff;
        }

        /* Store the remaining high MAC values */
        if(i == 1){
            addr[4] = (value >> 0)  & 0xff;
            addr[5] = (value >> 8)  & 0xff;
        }
    }
}


/**
 * @ingroup lan7800
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
    wait(lan7800_attached[minor]);
    signal(lan7800_attached[minor]);
    lan7800_isattached = 1;
    return USB_STATUS_SUCCESS;
}

/* LAN7800用の etherInit() の実装; この関数に関するドキュメントは
 * ether.h を参照 */
/**
 * @details
 *
 * LAN7800-固有の注記:  この関数はEthernetドライバがUSBコアに
 * 正常に登録された場合は ::OK; そうでない場合は ::SYSERR を返す。
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

    /* Get the MAC address and store it into addr[] */
    getEthAddr(ethptr->devAddress);

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
