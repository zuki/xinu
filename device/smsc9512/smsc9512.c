/**
 * @file     smsc9512.c
 *
 * このファイルはSMSC LAN9512 USB Ethernetドライバに日露な
 * 様々な関数を提供する.
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include "smsc9512.h"
#include <usb_core_driver.h>

/**
 * @ingroup ether_lan9512
 *
 * SMSC LAN9512 USB Ethernetアダプタのレジスタに書き出す.
 *
 * @param udev
 *      このアダプタ用のUSBデバイス
 * @param index
 *      書き出すレジスタのインデックス
 * @param data
 *      レジスタに書き出す値
 *
 * @return
 *      成功した場合は ::USB_STATUS_SUCCES; それ以外あh ::usb_status_t
 *      エラーコード
 */
usb_status_t
smsc9512_write_reg(struct usb_device *udev, uint32_t index, uint32_t data)
{
    return usb_control_msg(udev, NULL,
                           SMSC9512_VENDOR_REQUEST_WRITE_REGISTER,
                           USB_BMREQUESTTYPE_DIR_OUT |
                               USB_BMREQUESTTYPE_TYPE_VENDOR |
                               USB_BMREQUESTTYPE_RECIPIENT_DEVICE,
                           0, index, &data, sizeof(uint32_t));
}

/**
 * @ingroup ether_lan9512
 *
 * SMSC LAN9512 USB Ethernetアダプタのレジスタから読み込む.
 *
 * @param udev
 *      このアダプタ用のUSBデバイス
 * @param index
 *      読み込むレジスタのインデックス
 * @param data
 *      レジスタの値を書き込む場所へのポインタ
 *
 * @return
 *      成功した場合は ::USB_STATUS_SUCCES; それ以外あh ::usb_status_t
 *      エラーコード
 */
usb_status_t
smsc9512_read_reg(struct usb_device *udev, uint32_t index, uint32_t *data)
{
    return usb_control_msg(udev, NULL,
                           SMSC9512_VENDOR_REQUEST_READ_REGISTER,
                           USB_BMREQUESTTYPE_DIR_IN |
                               USB_BMREQUESTTYPE_TYPE_VENDOR |
                               USB_BMREQUESTTYPE_RECIPIENT_DEVICE,
                           0, index, data, sizeof(uint32_t));
}

/**
 * @ingroup ether_lan9512
 *
 * SMSC LAN9512 USB Ethernetアダプトのレジスタにある値を変更する.
 *
 * @param udev
 *      このアダプタ用のUSBデバイス
 * @param index
 *      変更するレジスタのインデックス
 * @param mask
 *      レジスタの古い値をクリアせず保持する位置のビットを1としたマスク
 *      （これらのビットが @p set に現れる場合は除く。その場合はセットされる）
 * @param set
 *      レジスタにセットするためのビットのマスク
 *
 * @return
 *      成功した場合は ::USB_STATUS_SUCCES; それ以外は ::usb_status_t
 *      エラーコード
 */
usb_status_t
smsc9512_modify_reg(struct usb_device *udev, uint32_t index,
                    uint32_t mask, uint32_t set)
{
    usb_status_t status;
    uint32_t val;

    status = smsc9512_read_reg(udev, index, &val);
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }
    val &= mask;
    val |= set;
    return smsc9512_write_reg(udev, index, val);
}

/**
 * @ingroup ether_lan9512
 *
 * SMSC LAN9512 USB Ethernetアダプタのレジスタのビットをセットする.
 *
 * @param udev
 *      このアダプタ用のUSBデバイス
 * @param index
 *      変更するレジスタのインデックス
 * @param set
 *      レジスタ内のセットするビット。 0がある位置には古い値が書き出される。
 *
 * @return
 *      成功した場合は ::USB_STATUS_SUCCES; それ以外あh ::usb_status_t
 *      エラーコード
 */
usb_status_t
smsc9512_set_reg_bits(struct usb_device *udev, uint32_t index, uint32_t set)
{
    return smsc9512_modify_reg(udev, index, 0xffffffff, set);
}

/**
 * @ingroup ether_lan9512
 *
 * SMSC LAN9512 USB EthernetアダプタのMACアドレスを変更する.
 *
 * @param udev
 *      このアダプタ用のUSBデバイス
 * @param macaddr
 *      セットする新しいMACアドレス（6バイト長）
 *
 * @return
 *      成功した場合は ::USB_STATUS_SUCCES; それ以外あh ::usb_status_t
 *      エラーコード。失敗の場合は既存のMACアドレスの一部が変更される可能性が
 *      ある。
 */
usb_status_t
smsc9512_set_mac_address(struct usb_device *udev, const uint8_t *macaddr)
{
    usb_status_t status;
    uint32_t addrl, addrh;

    addrl = macaddr[0] | macaddr[1] << 8 | macaddr[2] << 16 | macaddr[3] << 24;
    addrh = macaddr[4] | macaddr[5] << 8;

    status = smsc9512_write_reg(udev, ADDRL, addrl);
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }
    return smsc9512_write_reg(udev, ADDRH, addrh);
}

/**
 * @ingroup ether_lan9512
 *
 * SMSC LAN9512 USB EthernetアダプタのMACアドレスを読み込む.
 *
 * @param udev
 *      このアダプタ用のUSBデバイス
 * @param macaddr
 *      MACアドレスを書き出す場所へのポインタ（6バイト長）
 *
 * @return
 *      成功した場合は ::USB_STATUS_SUCCES; それ以外あh ::usb_status_t
 *      エラーコード
 */
usb_status_t
smsc9512_get_mac_address(struct usb_device *udev, uint8_t *macaddr)
{
    usb_status_t status;
    uint32_t addrl, addrh;

    status = smsc9512_read_reg(udev, ADDRL, &addrl);
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }
    status = smsc9512_read_reg(udev, ADDRH, &addrh);
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }
    macaddr[0] = (addrl >> 0)  & 0xff;
    macaddr[1] = (addrl >> 8)  & 0xff;
    macaddr[2] = (addrl >> 16) & 0xff;
    macaddr[3] = (addrl >> 24) & 0xff;
    macaddr[4] = (addrh >> 0)  & 0xff;
    macaddr[5] = (addrh >> 8)  & 0xff;
    return USB_STATUS_SUCCESS;
}
