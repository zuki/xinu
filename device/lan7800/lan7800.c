/**
 * @file     lan7800.c
 *
 * このファイルはLAN7800 USB Ethernetドライバに必要な様々な関数を提供する.
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <stdint.h>
#include <usb_core_driver.h>
#include <system/arch/arm/rpi-mailbox.h>
#include "lan7800.h"

/**
 * @ingroup ether_lan7800
 *
 * LAN7800 USB Ethernetアダプタのレジスタに書き出す.
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
usb_status_t lan7800_write_reg(struct usb_device *udev, uint32_t index, uint32_t data)
{
    uint32_t temp = data;

    return usb_control_msg(udev, 0,
                            USB_VENDOR_REQUEST_WRITE_REGISTER,
                            USB_BMREQUESTTYPE_DIR_OUT |
                            USB_BMREQUESTTYPE_TYPE_VENDOR |
                            USB_BMREQUESTTYPE_RECIPIENT_DEVICE,
                            0, index, &temp, sizeof(uint32_t));
}

/**
 * @ingroup ether_lan7800
 *
 * LAN7800 USB Ethernetアダプタのレジスタから読み込む.
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
usb_status_t lan7800_read_reg(struct usb_device *udev, uint32_t index, uint32_t *data)
{
    return usb_control_msg(udev, 0,
                            USB_VENDOR_REQUEST_READ_REGISTER,
                            USB_BMREQUESTTYPE_DIR_IN |
                            USB_BMREQUESTTYPE_TYPE_VENDOR |
                            USB_BMREQUESTTYPE_RECIPIENT_DEVICE,
                            0, index, data, sizeof(uint32_t));
}

/**
 * @ingroup ether_lan7800
 *
 * LAN7800 USB Ethernetアダプトのレジスタにある値を変更する.
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
lan7800_modify_reg(struct usb_device *udev, uint32_t index,
                    uint32_t mask, uint32_t set)
{
    usb_status_t status;
    uint32_t val;

    status = lan7800_read_reg(udev, index, &val);
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }
    val &= mask;
    val |= set;
    return lan7800_write_reg(udev, index, val);
}

/**
 * @ingroup ether_lan7800
 *
 * LAN7800 USB Ethernetアダプタのレジスタのビットをセットする.
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
lan7800_set_reg_bits(struct usb_device *udev, uint32_t index, uint32_t set)
{
    return lan7800_modify_reg(udev, index, 0xffffffff, set);
}

/**
 * @ingroup ether_lan7800
 *
 * LAN7800 USB EthernetアダプタのMACアドレスをセットする.
 *
 * @param udev
 *      このアダプタ用のUSBデバイス
 * @param macaddr
 *      セットする新しいMACアドレス（6バイト長）
 *
 * @return
 *      成功した場合は ::USB_STATUS_SUCCES; それ以外は ::usb_status_t
 *      エラーコード。失敗の場合は既存のMACアドレスの一部が変更される可能性が
 *      ある。
 */
usb_status_t lan7800_set_mac_address(struct usb_device *udev, const uint8_t *macaddr)
{
    usb_status_t status;
    uint32_t addrl, addrh;

    addrl = macaddr[0] | macaddr[1] << 8 | macaddr[2] << 16 | macaddr[3] << 24;
    addrh = macaddr[4] | macaddr[5] << 8;

    status = lan7800_write_reg(udev, RX_ADDRL, addrl);
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }
    status = lan7800_write_reg(udev, RX_ADDRH, addrh);
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }

    status = lan7800_write_reg(udev, MAF_LO(0), addrl);
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }
    return lan7800_write_reg(udev, MAF_LO(0), addrh | MAF_HI_VALID_);

}

/**
 * @ingroup ether_lan7800
 *
 * LAN7800 USB EthernetアダプタのMACアドレスを読み込む.
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
usb_status_t lan7800_get_mac_address(struct usb_device *udev, uint8_t *macaddr)
{
    usb_status_t status;
    uint32_t addrl, addrh;

    status = lan7800_read_reg(udev, RX_ADDRL, &addrl);
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }
    status = lan7800_read_reg(udev, RX_ADDRH, &addrh);
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

/**
 * @ingroup ether_lan7800
 *
 * LAN7800 USB Ethernetアダプタのループバックモードをオン/オフする.
 *
 * @param udev
 *      このアダプタ用のUSBデバイス
 * @param on_off
 *      ループバックモードのオン(1)/オフ(0)
 *
 * @return
 *      成功した場合は ::USB_STATUS_SUCCES; それ以外は ::usb_status_t
 *      エラーコード。失敗の場合は既存のMACアドレスの一部が変更される可能性が
 *      ある。
 */
usb_status_t lan7800_set_loopback_mode(struct usb_device *udev, unsigned int on_off)
{
    usb_status_t status;
    uint32_t val;

    status = lan7800_read_reg(udev, MAC_CR, &val);
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }
    val &= ~MAC_CR_LOOPBACK_;
    val |= (on_off == 1) ? MAC_CR_LOOPBACK_ : 0;

    status = lan7800_write_reg(udev, MAC_CR, val);

    return status;
}
