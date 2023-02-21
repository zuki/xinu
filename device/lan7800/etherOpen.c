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

#define jiffies                     (clkcount())

static int lan7800_read_raw_otp(struct usb_device *udev, uint32_t offset,
    uint32_t length, uint8_t *data)
{
    int i;
    uint32_t buf;
    uint64_t timeout;

    lan7800_read_reg(udev, OTP_PWR_DN, &buf);

    if (buf & OTP_PWR_DN_PWRDN_N_) {
        /* OTP_PWR_DN をクリアして、クリアされるのを待つ */
        lan7800_write_reg(udev, OTP_PWR_DN, 0);
        timeout = jiffies + platform.clkfreq;
        do {
            udelay(100);
            lan7800_read_reg(udev, OTP_PWR_DN, &buf);
            if (jiffies > timeout) {
                usb_dev_debug(udev, "timeout on OTP_PWR_DN\n");
                return -EIO;
            }
        } while (buf & OTP_PWR_DN_PWRDN_N_);
    }

    for (i = 0; i < length; i++) {
        lan7800_write_reg(udev, OTP_ADDR1, ((offset + i) >> 8) & OTP_ADDR1_15_11);
        lan7800_write_reg(udev, OTP_ADDR2, ((offset + i) & OTP_ADDR2_10_3));
        lan7800_write_reg(udev, OTP_FUNC_CMD, OTP_FUNC_CMD_READ_);
        lan7800_write_reg(udev, OTP_CMD_GO, OTP_CMD_GO_GO_);

        timeout = jiffies + platform.clkfreq;
        do {
            udelay(1);
            lan7800_read_reg(udev, OTP_STATUS, &buf);
            if (jiffies > timeout) {
                usb_dev_debug(udev, "timeout on OTP_STATUS\n");
                return -EIO;
            }
        } while (buf & OTP_STATUS_BUSY_);

        lan7800_read_reg(udev, OTP_RD_DATA, &buf);
        data[i] = (uint8_t)(buf & 0xFF);
    }

    return 0;
}


static int lan7800_read_otp (struct usb_device *udev, uint32_t offset,
    uint32_t length, uint8_t *data)
{
    uint8_t sig;
    int ret;

    ret = lan7800_read_raw_otp(udev, 0, 1, &sig);

    if (ret == 0) {
        if (sig == OTP_INDICATOR_1)
            offset = offset;
        else if (sig == OTP_INDICATOR_2)
            offset += 0x100;
        else
            ret = -EINVAL;
        if (!ret)
            ret = lan7800_read_raw_otp(udev, offset, length, data);
    }

    return ret;
}

static int lan7800_eeprom_confirm_not_busy(struct usb_device *udev)
{
    uint64_t timeout = jiffies + platform.clkfreq;
    uint32_t val;
    int ret;

    do {
        ret = lan7800_read_reg(udev, E2P_CMD, &val);
        if (ret < 0)
            return -EIO;

        if (!(val & E2P_CMD_EPC_BUSY_))
            return 0;

        udelay(40);
    } while (jiffies < timeout);

    usb_dev_debug(udev, "EEPROM is busy\n");
    return -EIO;
}

static int lan7800_wait_eeprom(struct usb_device *udev)
{
    uint64_t timeout = jiffies + platform.clkfreq/10;
    uint32_t val;
    int ret;

    do {
        ret = lan7800_read_reg(udev, E2P_CMD, &val);
        if (ret < 0)
            return -EIO;

        if (!(val & E2P_CMD_EPC_BUSY_) ||
            (val & E2P_CMD_EPC_TIMEOUT_))
            break;
        udelay(40);
    } while (jiffies < timeout);

    if (val & (E2P_CMD_EPC_TIMEOUT_ | E2P_CMD_EPC_BUSY_)) {
        usb_dev_debug(udev, "EEPROM read operation timeout\n");
        return -EIO;
    }

    return 0;
}

static int lan7800_read_raw_eeprom(struct usb_device *udev, uint32_t offset,
    uint32_t length, uint8_t *data)
{
    uint32_t val;
    uint32_t saved;
    int i, ret;
    int retval;

    /* depends on chip, some EEPROM pins are muxed with LED function.
     * disable & restore LED function to access EEPROM.
     */
    ret = lan7800_read_reg(udev, HW_CFG, &val);
    saved = val;

    // No need for test it is 78xx on Pi3B+
    val &= ~(HW_CFG_LED1_EN_ | HW_CFG_LED0_EN_);
    ret = lan7800_write_reg(udev, HW_CFG, val);

    retval = lan7800_eeprom_confirm_not_busy(udev);
    if (retval)
        return retval;

    for (i = 0; i < length; i++) {
        val = E2P_CMD_EPC_BUSY_ | E2P_CMD_EPC_CMD_READ_;
        val |= (offset & E2P_CMD_EPC_ADDR_MASK_);
        ret = lan7800_write_reg(udev, E2P_CMD, val);
        if (ret < 0) {
            retval = -EIO;
            goto exit;
        }

        retval = lan7800_wait_eeprom(udev);
        if (retval < 0)
            goto exit;

        ret = lan7800_read_reg(udev, E2P_DATA, &val);
        if (ret < 0) {
            retval = -EIO;
            goto exit;
        }

        data[i] = (uint8_t)(val & 0xFF);
        offset++;
    }

    retval = 0;
exit:
    // Again no need for test it is 78xx on Pi3B+
    ret = lan7800_write_reg(udev, HW_CFG, saved);

    return retval;
}

static int lan7800_read_eeprom(struct usb_device *udev, uint32_t offset,
    uint32_t length, uint8_t *data)
{
    uint8_t sig;
    int ret;

    ret = lan7800_read_raw_eeprom(udev, 0, 1, &sig);
    if ((ret == 0) && (sig == EEPROM_INDICATOR))
        ret = lan7800_read_raw_eeprom(udev, offset, length, data);
    else
        ret = -EINVAL;

    return ret;
}

static void lan7800_init_ltm(struct usb_device *udev)
{
    int ret;
    uint32_t buf;
    uint32_t regs[6] = { 0 };

    ret = lan7800_read_reg(udev, USB_CFG1, &buf);
    if (buf & USB_CFG1_LTM_ENABLE_) {
        uint8_t temp[2];
        /* Get values from EEPROM first */
        if (lan7800_read_eeprom(udev, 0x3F, 2, temp) == 0) {
            if (temp[0] == 24) {
                ret = lan7800_read_raw_eeprom(udev,
                    temp[1] * 2,
                    24,
                    (uint8_t *)regs);
                if (ret < 0)
                    return;
            }
        } else if (lan7800_read_otp(udev, 0x3F, 2, temp) == 0) {
            if (temp[0] == 24) {
                ret = lan7800_read_raw_otp(udev,
                    temp[1] * 2,
                    24,
                    (uint8_t *)regs);
                if (ret < 0)
                    return;
            }
        }
    }

    lan7800_write_reg(udev, LTM_BELT_IDLE0, regs[0]);
    lan7800_write_reg(udev, LTM_BELT_IDLE1, regs[1]);
    lan7800_write_reg(udev, LTM_BELT_ACT0, regs[2]);
    lan7800_write_reg(udev, LTM_BELT_ACT1, regs[3]);
    lan7800_write_reg(udev, LTM_INACTIVE0, regs[4]);
    lan7800_write_reg(udev, LTM_INACTIVE1, regs[5]);
}

static int lan7800_set_rx_max_frame_length(struct usb_device *udev, int size)
{
    uint32_t buf;
    bool rxenabled;

    lan7800_read_reg(udev, MAC_RX, &buf);

    rxenabled = ((buf & MAC_RX_RXEN_) != 0);

    if (rxenabled) {
        buf &= ~MAC_RX_RXEN_;
        lan7800_write_reg(udev, MAC_RX, buf);
    }

    /* add 4 to size for FCS */
    buf &= ~MAC_RX_MAX_SIZE_MASK_;
    buf |= (((size + 4) << MAC_RX_MAX_SIZE_SHIFT_) & MAC_RX_MAX_SIZE_MASK_);

    lan7800_write_reg(udev, MAC_RX, buf);

    if (rxenabled) {
        buf |= MAC_RX_RXEN_;
        lan7800_write_reg(udev, MAC_RX, buf);
    }

    return 0;
}

#define NETIF_F_RXCSUM                4
#define NETIF_F_HW_VLAN_CTAG_RX        2
#define NETIF_F_HW_VLAN_CTAG_FILTER 1

/* Enable or disable Rx checksum offload engine */
static int lan7800_set_features(struct usb_device *udev, uint32_t features)
{
    uint32_t rfe_ctl;
    lan7800_read_reg(udev, RFE_CTL, &rfe_ctl);

    if (features & NETIF_F_RXCSUM) {
        rfe_ctl |= RFE_CTL_TCPUDP_COE_ | RFE_CTL_IP_COE_;
        rfe_ctl |= RFE_CTL_ICMP_COE_ | RFE_CTL_IGMP_COE_;
    } else {
        rfe_ctl &= ~(RFE_CTL_TCPUDP_COE_ | RFE_CTL_IP_COE_);
        rfe_ctl &= ~(RFE_CTL_ICMP_COE_ | RFE_CTL_IGMP_COE_);
    }

    if (features & NETIF_F_HW_VLAN_CTAG_RX)
        rfe_ctl |= RFE_CTL_VLAN_STRIP_;
    else
        rfe_ctl &= ~RFE_CTL_VLAN_STRIP_;

    if (features & NETIF_F_HW_VLAN_CTAG_FILTER)
        rfe_ctl |= RFE_CTL_VLAN_FILTER_;
    else
        rfe_ctl &= ~RFE_CTL_VLAN_FILTER_;


    lan7800_write_reg(udev, RFE_CTL, rfe_ctl);

    return 0;
}

static int lan7800_reset(struct usb_device *udev, uint8_t* macaddress)
{
    int ret = 0;
    uint32_t buf;

    lan7800_read_reg(udev, HW_CFG, &buf);
    buf |= HW_CFG_LRST_;
    lan7800_write_reg(udev, HW_CFG, buf);

    uint64_t timeout = jiffies + platform.clkfreq;
    do {
        udelay(1);
        ret = lan7800_read_reg(udev, HW_CFG, &buf);
        if (jiffies > timeout)
        {
            usb_dev_debug(udev, "timeout on completion of LiteReset\n");
            return -EIO;
        }
    } while (buf & HW_CFG_LRST_);


    lan7800_set_mac_address(udev, macaddress);

    /* Respond to the IN token with a NAK */
    ret = lan7800_read_reg(udev, USB_CFG0, &buf);
    buf |= USB_CFG_BIR_;
    ret = lan7800_write_reg(udev, USB_CFG0, buf);

    /* Init LTM */
    lan7800_init_ltm(udev);

    //buf = DEFAULT_BURST_CAP_SIZE / HS_USB_PKT_SIZE;
    //buf = DEFAULT_BURST_CAP_SIZE / SS_USB_PKT_SIZE;
    buf = DEFAULT_BURST_CAP_SIZE / FS_USB_PKT_SIZE;
    ret = lan7800_write_reg(udev, BURST_CAP, buf);
    ret = lan7800_write_reg(udev, BULK_IN_DLY, DEFAULT_BULK_IN_DELAY);

    ret = lan7800_read_reg(udev, HW_CFG, &buf);
    buf |= HW_CFG_MEF_;
    buf |= HW_CFG_LED0_EN_;
    buf |= HW_CFG_LED1_EN_;
    ret = lan7800_write_reg(udev, HW_CFG, buf);

    ret = lan7800_read_reg(udev, USB_CFG0, &buf);
    buf |= USB_CFG_BCE_;
    ret = lan7800_write_reg(udev, USB_CFG0, buf);

    /* set FIFO sizes */
    buf = (MAX_RX_FIFO_SIZE - 512) / 512;
    ret = lan7800_write_reg(udev, FCT_RX_FIFO_END, buf);

    buf = (MAX_TX_FIFO_SIZE - 512) / 512;
    ret = lan7800_write_reg(udev, FCT_TX_FIFO_END, buf);

    ret = lan7800_write_reg(udev, INT_STS, INT_STS_CLEAR_ALL_);
    ret = lan7800_write_reg(udev, FLOW, 0);
    ret = lan7800_write_reg(udev, FCT_FLOW, 0);

    /* Don't need rfe_ctl_lock during initialisation */
    ret = lan7800_read_reg(udev, RFE_CTL, &buf);
    buf |= (RFE_CTL_BCAST_EN_ | RFE_CTL_DA_PERFECT_);
    ret = lan7800_write_reg(udev, RFE_CTL, buf);

    /* Enable or disable checksum offload engines */
    lan7800_set_features(udev, 0);

    //lan7800_set_multicast(dev->net);
    lan7800_read_reg(udev, RFE_CTL, &buf);
    buf &= ~(RFE_CTL_UCAST_EN_ | RFE_CTL_MCAST_EN_ |
        RFE_CTL_DA_PERFECT_ | RFE_CTL_MCAST_HASH_);
    ret = lan7800_write_reg(udev, RFE_CTL, buf);

    /* reset PHY */
    ret = lan7800_read_reg(udev, PMT_CTL, &buf);
    buf |= PMT_CTL_PHY_RST_;
    ret = lan7800_write_reg(udev, PMT_CTL, buf);

    timeout = jiffies + platform.clkfreq;
    do {
        udelay(100);
        ret = lan7800_read_reg(udev, PMT_CTL, &buf);
        if (jiffies > timeout) {
            usb_dev_debug(udev, "timeout waiting for PHY Reset\n");
            return -EIO;
        }
    } while ((buf & PMT_CTL_PHY_RST_) || !(buf & PMT_CTL_READY_));

    ret = lan7800_read_reg(udev, MAC_CR, &buf);


    uint8_t sig;
    ret = lan7800_read_raw_eeprom(udev, 0, 1, &sig);
    if (!ret && sig != EEPROM_INDICATOR) {
        usb_dev_debug(udev, "No External EEPROM. Setting MAC Speed\n");
        buf |= MAC_CR_AUTO_DUPLEX_ | MAC_CR_AUTO_SPEED_;
    }

    ret = lan7800_write_reg(udev, MAC_CR, buf);

    ret = lan7800_read_reg(udev, MAC_TX, &buf);
    buf |= MAC_TX_TXEN_;
    ret = lan7800_write_reg(udev, MAC_TX, buf);

    ret = lan7800_read_reg(udev, FCT_TX_CTL, &buf);
    buf |= FCT_TX_CTL_EN_;
    ret = lan7800_write_reg(udev, FCT_TX_CTL, buf);

    ret = lan7800_set_rx_max_frame_length(udev, ETH_MTU + ETH_VLAN_LEN);

    ret = lan7800_read_reg(udev, MAC_RX, &buf);
    buf |= MAC_RX_RXEN_;
    ret = lan7800_write_reg(udev, MAC_RX, buf);

    ret = lan7800_read_reg(udev, FCT_RX_CTL, &buf);
    buf |= FCT_RX_CTL_EN_;
    ret = lan7800_write_reg(udev, FCT_RX_CTL, buf);

    return 0;
}

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

    /* USBデバイスが実際に接続されるのを待つ  */
    if (lan7800_wait_device_attached(devptr->minor) != USB_STATUS_SUCCESS)
    {
        goto out_restore;
    }

    /* デバイスがdownしていない場合は失敗  */
    ethptr = &ethertab[devptr->minor];
    if (ethptr->state != ETH_STATE_DOWN)
    {
        goto out_restore;
    }

    /* Tx転送用のバッファプールを作成する  */
    ethptr->outPool = bfpalloc(sizeof(struct usb_xfer_request) +
                            ETH_MAX_PKT_LEN + TX_OVERHEAD,
                            MAX_TX_REQUESTS);
    if (ethptr->outPool == SYSERR)
    {
        goto out_restore;
    }

    /* Rxパケット用のバッファプールを作成する（実際のUSB転送用ではない。
     * それは別に割り当てられる） */
    ethptr->inPool = bfpalloc(sizeof(struct ethPktBuffer) + ETH_MAX_PKT_LEN,
                              ETH_IBLEN);
    if (ethptr->inPool == SYSERR)
    {
        goto out_free_out_pool;
    }

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

    if (lan7800_reset(udev, &ethptr->devAddress[0]) < 0) {
        goto out_free_in_pool;
    }

    /* MACアドレスをセットする
    if (lan7800_set_mac_address(udev, ethptr->devAddress) != USB_STATUS_SUCCESS)
    {
        goto out_free_in_pool;
    }
    */

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
