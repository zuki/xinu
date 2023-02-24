/**
 * @file     lan7800.c
 *
 * このファイルはLAN7800 USB Ethernetドライバに必要な様々な関数を提供する.
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <stdint.h>
#include <usb_core_driver.h>
#include <ether.h>
#include <clock.h>
#include <system/platforms/arm-rpi3/rpi-mailbox.h>
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
    return usb_control_msg(udev, NULL,
                            USB_VENDOR_REQUEST_WRITE_REGISTER,
                            USB_BMREQUESTTYPE_DIR_OUT |
                            USB_BMREQUESTTYPE_TYPE_VENDOR |
                            USB_BMREQUESTTYPE_RECIPIENT_DEVICE,
                            0, index, &data, sizeof(uint32_t));
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
    return usb_control_msg(udev, NULL,
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
 * ビット値が変更されるまで待つ
 * @param udev  アダプタのUSBデバイス
 * @param reg   変更するレジスタ
 * @param mask  レジスタのマスク値
 * @param set   適用するレジスタビットの値
 * @return 値が適用されたら ::USB_STATUS_SUCCESS
 */
usb_status_t lan7800_mdio_wait_for_bit(struct usb_device *udev,
    const uint32_t reg, const uint32_t mask, const bool set)
{
    uint32_t val = 0;
    while(1) {
        lan7800_read_reg(udev, reg, &val);

        if (!set)
            val = ~val;
        if ((val & mask) == mask)
            return USB_STATUS_SUCCESS;
        udelay(1);
    }
}

/**
 * @ingroup ether_lan7800
 *
 * PHYが開放されるのを待つ.
 * @param udev  アダプタのUSBデバイス
 * @return 成功したら ::USB_STATUS_SUCCESS
 */
usb_status_t lan7800_phy_wait_not_busy(struct usb_device *udev)
{
    return lan7800_mdio_wait_for_bit(udev, MII_ACC,
            MII_ACC_MII_BUSY_, USB_STATUS_SUCCESS);
}

/**
 * @ingroup ether_lan7800
 *
 * 待つ前にEEPROMがビジーでないことを確認する. （ヘルパー関数）
 * @param udev  アダプタのUSBデバイス
 * @return 成功したら ::USB_STATUS_SUCCESS
 */
usb_status_t lan7800_eeprom_confirm_not_busy(struct usb_device *udev)
{
    return lan7800_wait_for_bit(udev, E2P_CMD,
            E2P_CMD_EPC_BUSY_, USB_STATUS_SUCCESS);
}

/**
 * @ingroup ether_lan7800
 *
 * EEPROMを待つ. (ヘルパー関数)
 * @param udev  アダプタのUSBデバイス
 * @return 成功したら ::USB_STATUS_SUCCESS
 */
usb_status_t lan7800_wait_eeprom(struct usb_device *udev)
{
    return lan7800_wait_for_bit(udev, E2P_CMD,
            (E2P_CMD_EPC_BUSY_ | E2P_CMD_EPC_TIMEOUT_),
            USB_STATUS_SUCCESS);
}

/**
 * @ingroup ether_lan7800
 *
 * 生のEEPROM値を読み込む.
 * @param udev  アダプタのUSBデバイス
 * @return 成功したら ::USB_STATUS_SUCCESS
 */
usb_status_t lan7800_read_raw_eeprom(struct usb_device *udev,
    uint32_t offset, uint32_t length, uint8_t *data)
{
    uint32_t val;
    uint32_t saved;
    int i, retval;

    lan7800_read_reg(udev, HW_CFG, &val);
    saved = val;

    /* EEPROMにアクセスするためにLED機能を無効にして保管.
     * LAN780ではEEPROMとLED機能が多重化されているため. */
    val &= ~(HW_CFG_LED1_EN_ | HW_CFG_LED0_EN_);
    lan7800_write_reg(udev, HW_CFG, val);

    retval = lan7800_eeprom_confirm_not_busy(udev);
    if (retval)
        return retval;

    for (i = 0; i < length; i++) {
        val = E2P_CMD_EPC_BUSY_ | E2P_CMD_EPC_CMD_READ_;
        val |= (offset & E2P_CMD_EPC_ADDR_MASK_);
        retval = lan7800_write_reg(udev, E2P_CMD, val);
        if (retval < 0) {
            retval = -EIO;
            goto exit;
        }

        retval = lan7800_wait_eeprom(udev);
        if (retval < 0)
            goto exit;

        retval = lan7800_read_reg(udev, E2P_DATA, &val);
        if (retval < 0) {
            retval = -EIO;
            goto exit;
        }

        data[i] = (uint8_t)(val & 0xFF);
        offset++;
    }
    retval = USB_STATUS_SUCCESS;
exit:
    lan7800_write_reg(udev, HW_CFG, saved);

    return retval;
}

/**
 * @ingroup ether_lan7800
 *
 * 最大RXフレームサイズをセットする.
 * @param udev  アダプタのUSBデバイス
 * @param size  最大RXフレームサイズ
 * @return 成功したら ::USB_STATUS_SUCCESS
 */
usb_status_t lan7800_set_rx_max_frame_length(struct usb_device *udev, int size)
{
    uint32_t buf;
    bool rxenabled;

    lan7800_read_reg(udev, MAC_RX, &buf);

    rxenabled = ((buf & MAC_RX_RXEN_) != 0);

    if (rxenabled) {
        buf &= ~MAC_RX_RXEN_;
        lan7800_write_reg(udev, MAC_RX, buf);
    }

    /* To fit FCS, add 4 */
    buf &= ~MAC_RX_MAX_SIZE_MASK_;
    buf |= (((size + 4) << MAC_RX_MAX_SIZE_SHIFT_) & MAC_RX_MAX_SIZE_MASK_);

    lan7800_write_reg(udev, MAC_RX, buf);

    if (rxenabled) {
        buf |= MAC_RX_RXEN_;
        lan7800_write_reg(udev, MAC_RX, buf);
    }

    return USB_STATUS_SUCCESS;
}

/**
 * @ingroup ether_lan7800
 *
 * チェックサムオフロードエンジンを有効/無効にする.
 * @param udev  アダプタのUSBデバイス
 * @param set   ビット（有効/無効）
 * @return 成功したら ::USB_STATUS_SUCCESS
 */
usb_status_t lan7800_set_features(struct usb_device *udev, uint32_t set)
{
    uint32_t rfe_ctl;
    lan7800_read_reg(udev, RFE_CTL, &rfe_ctl);

    if (set & NETIF_F_RXCSUM) {
        rfe_ctl |= RFE_CTL_TCPUDP_COE_ | RFE_CTL_IP_COE_;
        rfe_ctl |= RFE_CTL_ICMP_COE_ | RFE_CTL_IGMP_COE_;
    }
    else {
        rfe_ctl &= ~(RFE_CTL_TCPUDP_COE_ | RFE_CTL_IP_COE_);
        rfe_ctl &= ~(RFE_CTL_ICMP_COE_ | RFE_CTL_IGMP_COE_);
    }

    if (set & NETIF_F_HW_VLAN_CTAG_RX)
        rfe_ctl |= RFE_CTL_VLAN_STRIP_;
    else
        rfe_ctl &= ~RFE_CTL_VLAN_STRIP_;

    if (set & NETIF_F_HW_VLAN_CTAG_FILTER)
        rfe_ctl |= RFE_CTL_VLAN_FILTER_;
    else
        rfe_ctl &= ~RFE_CTL_VLAN_FILTER_;

    lan7800_write_reg(udev, RFE_CTL, rfe_ctl);

    return USB_STATUS_SUCCESS;
}


/**
 * @ingroup ether_lan7800
 *
 * LAN7800 USB Ethernetアダプタのループバックモードをオン/オフする.
 *
 * @param udev
 *      アダプタのUSBデバイス
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

/**
 * @ingroup ether_lan7800
 *
 * Microchip LAN7800 Ethernetデバイスの様々な機能を初期化する.
 * @param udev          アダプタのUSBデバイス
 * @param macaddress    デバイスにセットするMACアドレス
 * @return 成功したら ::USB_STATUS_SUCCESS
 */
usb_status_t lan7800_init(struct usb_device *udev, uint8_t* macaddress)
{
    uint32_t buf;

    /* デバイスにMACアドレスをセットする */
    lan7800_set_mac_address(udev, macaddress);

    /* IN都県にNAKで応答する */
    lan7800_read_reg(udev, USB_CFG0, &buf);
    buf |= USB_CFG_BIR_;
    lan7800_write_reg(udev, USB_CFG0, buf);

    /* バーストキャップをセットする */
    buf = DEFAULT_BURST_CAP_SIZE / FS_USB_PKT_SIZE;
    lan7800_write_reg(udev, BURST_CAP, buf);
    lan7800_write_reg(udev, BULK_IN_DLY, DEFAULT_BULK_IN_DELAY);

    /* HW CFG経由でLEDを有効にする */
    lan7800_read_reg(udev, HW_CFG, &buf);
    buf |= HW_CFG_MEF_; /* Multiple ethernet frames */
    buf |= HW_CFG_LED0_EN_;
    buf |= HW_CFG_LED1_EN_;
    lan7800_write_reg(udev, HW_CFG, buf);

    lan7800_read_reg(udev, USB_CFG0, &buf);
    buf |= USB_CFG_BCE_;
    lan7800_write_reg(udev, USB_CFG0, buf);

    /* FIFOサイズをセットする（SMSC9512と同じ） */
    buf = (MAX_RX_FIFO_SIZE - 512) / 512;
    lan7800_write_reg(udev, FCT_RX_FIFO_END, buf);

    buf = (MAX_TX_FIFO_SIZE - 512) / 512;
    lan7800_write_reg(udev, FCT_TX_FIFO_END, buf);

    lan7800_write_reg(udev, INT_STS, INT_STS_CLEAR_ALL_);
    lan7800_write_reg(udev, FLOW, 0);
    lan7800_write_reg(udev, FCT_FLOW, 0);

    lan7800_read_reg(udev, RFE_CTL, &buf);
    buf |= (RFE_CTL_BCAST_EN_ | RFE_CTL_UCAST_EN_ | RFE_CTL_MCAST_EN_);
    lan7800_write_reg(udev, RFE_CTL, buf);

    /* チェックサムオフロードエンジンを無効にする */
    lan7800_set_features(udev, 0);

    /* パケットをフィルタリングしない。これはネットワークスタックで行う */
    lan7800_read_reg(udev, RFE_CTL, &buf);
    buf &= ~(RFE_CTL_DA_PERFECT_ | RFE_CTL_MCAST_HASH_);
    lan7800_write_reg(udev, RFE_CTL, buf);

    lan7800_read_reg(udev, MAC_CR, &buf);

    /* MAC スピードをセット */
    uint8_t sig;
    lan7800_read_raw_eeprom(udev, 0, 1, &sig);
    if (sig != EEPROM_INDICATOR) {
        usb_dev_debug(udev, "No External EEPROM. Setting MAC Speed\n");
        buf |= MAC_CR_AUTO_DUPLEX_ | MAC_CR_AUTO_SPEED_;
    }

    buf &= ~(MAC_CR_AUTO_DUPLEX_);
    lan7800_write_reg(udev, MAC_CR, buf);

    /* Full duplexモード */
    lan7800_read_reg(udev, MAC_CR, &buf);
    buf |= (1 << 3);
    lan7800_write_reg(udev, MAC_CR, buf);

    /* MAC, FCT用のTX, RXをセット (SMSC9512と同じ) */
    lan7800_read_reg(udev, MAC_TX, &buf);
    buf |= MAC_TX_TXEN_;
    lan7800_write_reg(udev, MAC_TX, buf);

    lan7800_read_reg(udev, FCT_TX_CTL, &buf);
    buf |= FCT_TX_CTL_EN_;
    lan7800_write_reg(udev, FCT_TX_CTL, buf);

    lan7800_set_rx_max_frame_length(udev, ETH_MTU + ETH_VLAN_LEN);

    lan7800_read_reg(udev, MAC_RX, &buf);
    buf |= MAC_RX_RXEN_;
    lan7800_write_reg(udev, MAC_RX, buf);

    lan7800_read_reg(udev, FCT_RX_CTL, &buf);
    buf |= FCT_RX_CTL_EN_;
    lan7800_write_reg(udev, FCT_RX_CTL, buf);

    return USB_STATUS_SUCCESS;
}
