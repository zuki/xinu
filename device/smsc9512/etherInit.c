/**
 * @file etherInit.c
 *
 * SMSC9512 USB Ethernetアダプタを初期化する
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include "smsc9512.h"
#include <clock.h>
#include <ether.h>
#include <memory.h>
#include <platform.h>
#include <semaphore.h>
#include <stdlib.h>
#include <usb_core_driver.h>

/* Ethernetデバイスのグローバルテーブル  */
struct ether ethertab[NETHER];

/**
 * 指定されたEthernetデバイスが実際にまだシステム（ここではUSB）に
 * 接続されているか否かを示すセマフォ。他のドライバが必要であれば
 * <code>struct ::ether</code> に移動させるかもしれない。
 */
static semaphore smsc9512_attached[NETHER];

/**
 * 指定されたUSBデバイスへのSMSC LAN9512ドライバのバインドを試みる。
 * @ref usb_device_driver::bind_device "bind_device" の
 * SMSC LAN9512ドライバ用の実装であり、そのドキュメントに記載されて
 * いる動作に準拠している。
 */
static usb_status_t
smsc9512_bind_device(struct usb_device *udev)
{
    struct ether *ethptr;

    /* USBコアが既にメモリに読み込んでいるUSBデバイスの標準デバイス
     * ディスクリプタを確認して、これが実際にSMSC LAN9512であることを確認する。
     * また、パケットの送受信に必要なエンドポイントが存在するか、デバイスが
     * High Speedで動作するかも確認する。
     */
    if (udev->descriptor.idVendor != SMSC9512_VENDOR_ID ||
        udev->descriptor.idProduct != SMSC9512_PRODUCT_ID ||
        udev->interfaces[0]->bNumEndpoints < 2 ||
        (udev->endpoints[0][0]->bmAttributes & 0x3) != USB_TRANSFER_TYPE_BULK ||
        (udev->endpoints[0][1]->bmAttributes & 0x3) != USB_TRANSFER_TYPE_BULK ||
        (udev->endpoints[0][0]->bEndpointAddress >> 7) != USB_DIRECTION_IN ||
        (udev->endpoints[0][1]->bEndpointAddress >> 7) != USB_DIRECTION_OUT ||
        udev->speed != USB_SPEED_HIGH)
    {
        return USB_STATUS_DEVICE_UNSUPPORTED;
    }

    /* このデバイスがすでにSMSC LAN9512にバインドされていないか確認する。
     * TODO: このタイプの複数のデバイスを並行してサポートする  */
    ethptr = &ethertab[0];
    STATIC_ASSERT(NETHER == 1);
    if (ethptr->csr != NULL)
    {
        return USB_STATUS_DEVICE_UNSUPPORTED;
    }

    /* この関数の残りの部分ではSMSC LAN9512を使用可能な状態にするが
     * 実際にRxとTxを有効にすることはない（これはetherOpen() で行う）。
     * 個々での作業は主にSMSC LAN9512のレジスタへの書き込むである。
     * ただし、これはUSBに接続されたUSBイーサネットアダプタなので
     * メモリマップドレジスタではない。レジスタの読み書きはUSBの
     * コントロール転送を利用して行われる。少し面倒であり、メモリ
     * アクセスと違ってUSBコントロール転送は失敗する可能性もある。
     * しかし、ここでは必要な読み書きをすべて行い、最後にエラーが
     * 発生したか否をチェックする遅延エラーチェックを行っている。
     */

    udev->last_error = USB_STATUS_SUCCESS;

    /* SMSC LAN9512をレジスタ軽腕リセットする必要はないはずである。
     * USBコードがLAN9512が接続されたUSBポートのリセットを既に実行
     * しているからである
     */

    /* MACアドレスをセットする */
    smsc9512_set_mac_address(udev, ethptr->devAddress);

    /* 1回のUSB転送で複数のEthernetフレームを受信できるようにする。
     * また、機能不明のフラグをいくつかセットする。 */
    smsc9512_set_reg_bits(udev, HW_CFG, HW_CFG_MEF | HW_CFG_BIR | HW_CFG_BCE);

    /* USB Rx転送あたりの最大USB（ネットワークではない！）パケットをセットする。
     * HW_CFG_MEFが設定された場合に必要になる */
    smsc9512_write_reg(udev, BURST_CAP,
                       SMSC9512_DEFAULT_HS_BURST_CAP_SIZE / SMSC9512_LAN7800_HS_USB_PKT_SIZE);

    /* エラーをチェックして復帰する */
    if (udev->last_error != USB_STATUS_SUCCESS)
    {
        return udev->last_error;
    }
    ethptr->csr = udev;
    udev->driver_private = ethptr;
    signal(smsc9512_attached[ethptr - ethertab]);
    return USB_STATUS_SUCCESS;
}

/**
 * デタッチされたSMSC LAN9512から SMSC LAN9512ドライバをアンバインドする。
 * これはSMSC LAN9512 ドライバの @ref usb_device_driver::unbind_device
 * "unbind_device" の実装であり、ドキュメントに記載されている動作に準ずる。
 */
static void
smsc9512_unbind_device(struct usb_device *udev)
{
    struct ether *ethptr = udev->driver_private;

    /* アタッチされていたセマフォを0にリセットする  */
    wait(smsc9512_attached[ethptr - ethertab]);

    /* デバイスをクローズする  */
    etherClose(ethptr->dev);
}

/**
 * SMSC LAN9512のUSBデバイスドライバの仕様。これはUSBコアに特化したものであり、
 * Xinuの主要なデバイスとドライバの（静的な）モデルとは関係しない。
 */
static const struct usb_device_driver smsc9512_driver = {
    .name          = "SMSC LAN9512 USB Ethernet Adapter Driver",
    .bind_device   = smsc9512_bind_device,
    .unbind_device = smsc9512_unbind_device,
};

static void
randomEthAddr(uchar addr[ETH_ADDR_LEN])
{
    uint i;
    if (platform.serial_low != 0 && platform.serial_high != 0)
    {
        /* プラットフォームのシリアル番号から生成された値を使用する。
         * ここで問題となるのは、64ビットのシリアル番号から48ビットの
         * MACアドレスを生成しなければならないが、複数のシリアル番号を
         * 同じMACアドレスにマッピングすることは避けなければならないと
         * いう点である。これは不可能なので、シリアル番号をハッシュ化し、
         * シリアル番号の割り当て方法における明らかな非ランダム性を取り
         * 除き、下位48ビットを抽出することで近似を行うこととする。
         */
        unsigned long long serial_nr, hashval;

        serial_nr = (unsigned long long)platform.serial_low |
                    ((unsigned long long)platform.serial_high << 32);
        hashval = serial_nr * 0x9e37fffffffc0001ULL;
        for (i = 0; i < ETH_ADDR_LEN; i++)
        {
            addr[i] = hashval & 0xff;
            hashval >>= 8;
        }
    }
    else
    {
        /* Cライブラリの乱数生成器を使用し、現在のシステムタイマーの
         * ティックカウントを種として値を生成する  */
        srand(clkcount());
        for (i = 0; i < ETH_ADDR_LEN; i++)
        {
            addr[i] = rand();
        }
    }
    /* マルチキャストビットをクリアし、ローカルにアサインしたビットをセットする */
    addr[0] &= 0xfe;
    addr[0] |= 0x02;
}

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
smsc9512_wait_device_attached(ushort minor)
{
    wait(smsc9512_attached[minor]);
    signal(smsc9512_attached[minor]);
    return USB_STATUS_SUCCESS;
}

/* smsc9512用の etherInit() の実装; この関数に関するドキュメントは
 * ether.h を参照 */
/**
 * @details
 *
 * SMSC LAN9512-固有の注記:  この関数はEthernetドライバがUSBコアに
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

    smsc9512_attached[devptr->minor] = semcreate(0);

    if (isbadsem(smsc9512_attached[devptr->minor]))
    {
        goto err_free_isema;
    }

    /* Raspberry Piに搭載されているSMSC LAN9512にはEEPROMが付属していない。
     * 通常、EEPROMはアダプタのMACアドレスとその他の情報を格納するために
     * 使用される。そのため、ソフトウェアは（乱数などで）選択した任意の
     * MACアドレスを設定する必要がある。
     */
    randomEthAddr(ethptr->devAddress);

    /* このデバイスドライバをUSBコアに登録して復帰する */
    status = usb_register_device_driver(&smsc9512_driver);
    if (status != USB_STATUS_SUCCESS)
    {
        goto err_free_attached_sema;
    }
    return OK;

err_free_attached_sema:
    semfree(smsc9512_attached[devptr->minor]);
err_free_isema:
    semfree(ethptr->isema);
err:
    return SYSERR;
}
