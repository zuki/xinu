/**
 * @file     lan7800.h
 * @ingroup lan7800
 *
 * @authors
 *         Rade Latinovich
 *        Patrick J. McGee
 *
 * このヘッダーファイルはLAN7800 USB Ethernetアダプタのレジスタ定義を
 * 提供する. これらの定義の多くは主にMicrochip社のLAN78XX Linux Driverから
 * 借用している。
 */

#ifndef _LAN7800_H_
#define _LAN7800_H_

#include <stddef.h>
#include <stdint.h>
#include "usb_util.h"
#include <clock.h>

#define LAN7800_VENDOR_ID       0x0424
#define LAN7800_PRODUCT_ID      0x7800

/* The 5 basic operations on flags */
usb_status_t lan7800_write_reg(struct usb_device *udev, uint32_t index, uint32_t data);
usb_status_t lan7800_read_reg(struct usb_device *udev, uint32_t index, uint32_t *data);
usb_status_t lan7800_modify_reg(struct usb_device *udev, uint32_t index, uint32_t mask, uint32_t set);
usb_status_t lan7800_set_reg_bits(struct usb_device *udev, uint32_t index, uint32_t set);
usb_status_t lan7800_wait_device_attached(unsigned short minor);
usb_status_t lan7800_set_loopback_mode(struct usb_device *udev, unsigned int on_off);
usb_status_t lan7800_set_mac_address(struct usb_device *udev, const uint8_t *macaddr);
usb_status_t lan7800_get_mac_address(struct usb_device *udev, uint8_t *macaddr);

struct usb_xfer_request;

void lan7800_tx_complete(struct usb_xfer_request *req);
void lan7800_rx_complete(struct usb_xfer_request *req);

static inline void
__lan7800_dump_reg(struct usb_device *udev, uint32_t index, const char *name)
{
    uint32_t val = 0;
    lan7800_read_reg(udev, index, &val);
    kprintf("LAN7800: %s = 0x%08X\r\n", name, val);
}

#define lan7800_dump_reg(udev, index) __lan7800_dump_reg(udev, index, #index)

/* lan7800_mdio関数 */
usb_status_t  lan7800_mdio_read(struct usb_device *udev, int phy_id, int idx);
void lan7800_mdio_write(struct usb_device *udev, int phy_id, int idx, int regval);

/* LAN7800関数定義 (lan7800.cで定義)  */
usb_status_t lan7800_read_raw_otp(struct usb_device *udev,
        uint32_t offset, uint32_t length, uint8_t *data);
int lan7800_read_otp_mac(unsigned char *enetaddr, struct usb_device *udev);
usb_status_t lan7800_eeprom_confirm_not_busy(struct usb_device *udev);
usb_status_t lan7800_wait_eeprom(struct usb_device *udev);
usb_status_t lan7800_read_raw_eeprom(struct usb_device *dev,
        uint32_t offset, uint32_t length, uint8_t *data);
usb_status_t lan7800_read_eeprom(struct usb_device *dev, uint32_t offset,
        uint32_t length, uint8_t *data);
usb_status_t lan7800_set_rx_max_frame_length(struct usb_device *dev, int size);

#define NETIF_F_RXCSUM                  4
#define NETIF_F_LAN7800_HW_VLAN_CTAG_RX         2
#define NETIF_F_LAN7800_HW_VLAN_CTAG_FILTER     1
usb_status_t lan7800_set_features(struct usb_device *dev,
        uint32_t features);
usb_status_t lan7800_init(struct usb_device *dev, uint8_t* macaddress);

usb_status_t lan7800_mdio_wait_for_bit(struct usb_device *udev,
        const uint32_t reg, const uint32_t mask, const bool set);

#define lan7800_wait_for_bit    lan7800_mdio_wait_for_bit

/* ********************************************************************
 * 以下のレジスタ定義は主にLinuxのオープンソースの78xxドライバ:
 * https://github.com/torvalds/linux/blob/8fa3b6f9392bf6d90cb7b908e07bd90166639f0a/drivers/net/usb/lan78xx.h
 * と U-Boot:
 * https://github.com/trini/u-boot/blob/890e79f2b1c26c5ba1a86d179706348aec7feef7/drivers/usb/eth/lan7x.h
 * による
 * ******************************************************************** */

#define EIO                     5                // Error IO = 5
#define EINVAL                  22

#define TX_OVERHEAD             8
#define RX_OVERHEAD             10

/* Tx Command A */
#define TX_CMD_A_IGE_               0x20000000
#define TX_CMD_A_ICE_               0x10000000
#define TX_CMD_A_LSO_               0x08000000
#define TX_CMD_A_IPE_               0x04000000
#define TX_CMD_A_TPE_               0x02000000
#define TX_CMD_A_IVTG_              0x01000000
#define TX_CMD_A_RVTG_              0x00800000
#define TX_CMD_A_FCS_               0x00400000
#define TX_CMD_A_LEN_MASK_          0x000FFFFF

/* TX word A buffer size. */
#define LAN7800_TX_CMD_A_BUF_SIZE    0x000FFFFF

/* Tx Command B */
#define TX_CMD_B_MSS_SHIFT_         16
#define TX_CMD_B_MSS_MASK_          0x3FFF0000
#define TX_CMD_B_MSS_MIN_           (unsigned short)8
#define TX_CMD_B_VTAG_MASK_         0x0000FFFF
#define TX_CMD_B_VTAG_PRI_MASK_     0x0000E000
#define TX_CMD_B_VTAG_CFI_MASK_     0x00001000
#define TX_CMD_B_VTAG_VID_MASK_     0x00000FFF

/* Rx Command A */
#define RX_CMD_A_ICE_               0x80000000
#define RX_CMD_A_TCE_               0x40000000
#define RX_CMD_A_CSE_MASK_          0xC0000000
#define RX_CMD_A_IPV_               0x20000000
#define RX_CMD_A_PID_MASK_          0x18000000
#define RX_CMD_A_PID_NONE_IP_       0x00000000
#define RX_CMD_A_PID_TCP_IP_        0x08000000
#define RX_CMD_A_PID_UDP_IP_        0x10000000
#define RX_CMD_A_PID_IP_            0x18000000
#define RX_CMD_A_PFF_               0x04000000
#define RX_CMD_A_BAM_               0x02000000
#define RX_CMD_A_MAM_               0x01000000
#define RX_CMD_A_FVTG_              0x00800000
#define RX_CMD_A_RED_               0x00400000
#define RX_CMD_A_RX_ERRS_MASK_      0xC03F0000
#define RX_CMD_A_RWT_               0x00200000
#define RX_CMD_A_RUNT_              0x00100000
#define RX_CMD_A_LONG_              0x00080000
#define RX_CMD_A_RXE_               0x00040000
#define RX_CMD_A_DRB_               0x00020000
#define RX_CMD_A_FCS_               0x00010000
#define RX_CMD_A_UAM_               0x00008000
#define RX_CMD_A_ICSM_              0x00004000
#define RX_CMD_A_LEN_MASK_          0x00003FFF

/* Rx Command B */
#define RX_CMD_B_CSUM_SHIFT_        16
#define RX_CMD_B_CSUM_MASK_         0xFFFF0000
#define RX_CMD_B_VTAG_MASK_         0x0000FFFF
#define RX_CMD_B_VTAG_PRI_MASK_     0x0000E000
#define RX_CMD_B_VTAG_CFI_MASK_     0x00001000
#define RX_CMD_B_VTAG_VID_MASK_     0x00000FFF

/* Rx Command C */
#define RX_CMD_C_WAKE_SHIFT_        15
#define RX_CMD_C_WAKE_              0x8000
#define RX_CMD_C_REF_FAIL_SHIFT_    14
#define RX_CMD_C_REF_FAIL_          0x4000

/* linuxのドライバを変換する際に必要なプラットフォーム固有な値である */
#define LAN7800_TX_OVERHEAD         8
#define LAN7800_RX_OVERHEAD         4

#define LAN7800_FS_USB_PKT_SIZE     64
#define LAN7800_HS_USB_PKT_SIZE     512
#define LAN7800_SS_USB_PKT_SIZE     1024

#define LAN7800_MAX_TX_REQUESTS     1
#define LAN7800_MAX_RX_REQUESTS     1

#define LAN7800_MAX_RX_FIFO_SIZE        12 * 1024
#define LAN7800_MAX_TX_FIFO_SIZE        12 * 1024
#define LAN7800_DEFAULT_BURST_CAP_SIZE  LAN7800_MAX_TX_FIFO_SIZE
#define LAN7800_BULK_IN_DELAY           0x094
#define LAN7800_DEFAULT_BULK_IN_DELAY   0x800

#define LAN7800_INT_STS         0x00C
#define LAN7800_INT_STS_CLEAR_ALL_  0xFFFFFFFF
#define LAN7800_HW_CFG          0x010
#define LAN7800_HW_CFG_LED1_EN_     0x00200000
#define LAN7800_HW_CFG_LED0_EN_     0x00100000
#define LAN7800_HW_CFG_MEF_         0x00000010
#define LAN7800_HW_CFG_LRST_        0x00000002
#define LAN7800_PMT_CTL                 0x014
#define LAN7800_PMT_CTL_PHY_RST_            0x00000010
#define LAN7800_PMT_CTL_READY_              0x00000080
#define LAN7800_USB_CFG0        0x080
#define LAN7800_USB_CFG_BIR_        0x00000040
#define LAN7800_USB_CFG_BCE_        0x00000020
#define LAN7800_USB_CFG1        0x084
#define LAN7800_USB_CFG1_LAN7800_LTM_ENABLE_    0x00000100
#define LAN7800_BURST_CAP       0x090
#define LAN7800_BULK_IN_DLY     0x094
#define LAN7800_RFE_CTL         0x0B0
#define LAN7800_RFE_CTL_IGMP_COE_           0x00004000
#define LAN7800_RFE_CTL_ICMP_COE_           0x00002000
#define LAN7800_RFE_CTL_TCPUDP_COE_         0x00001000
#define LAN7800_RFE_CTL_IP_COE_             0x00000800
#define LAN7800_RFE_CTL_BCAST_EN_           0x00000400
#define LAN7800_RFE_CTL_MCAST_EN_           0x00000200
#define LAN7800_RFE_CTL_UCAST_EN_           0x00000100
#define LAN7800_RFE_CTL_VLAN_STRIP_         0x00000080
#define LAN7800_RFE_CTL_DISCARD_UNTAGGED_   0x00000040
#define LAN7800_RFE_CTL_VLAN_FILTER_        0x00000020
#define LAN7800_RFE_CTL_SA_FILTER_          0x00000010
#define LAN7800_RFE_CTL_MCAST_HASH_         0x00000008
#define LAN7800_RFE_CTL_DA_HASH_            0x00000004
#define LAN7800_RFE_CTL_DA_PERFECT_         0x00000002
#define LAN7800_RFE_CTL_RST_                0x00000001
#define LAN7800_FCT_RX_CTL      0x0C0
#define LAN7800_FCT_RX_CTL_EN_      0x80000000
#define LAN7800_FCT_TX_CTL      0x0C4
#define LAN7800_FCT_TX_CTL_EN_      0x80000000
#define LAN7800_FCT_RX_FIFO_END 0x0C8
#define LAN7800_FCT_TX_FIFO_END 0x0CC
#define LAN7800_FCT_FLOW        0x0D0
#define LAN7800_LTM_BELT_IDLE0  0x0E0
#define LAN7800_LTM_BELT_IDLE1  0x0E4
#define LAN7800_LTM_BELT_ACT0   0x0E8
#define LAN7800_LTM_BELT_ACT1   0x0EC
#define LAN7800_LTM_INACTIVE0   0x0F0
#define LAN7800_LTM_INACTIVE1   0x0F4
/* MAC制御 */
#define LAN7800_MAC_CR          0x100         ///< MAC CONTROL REGISTER
#define LAN7800_MAC_CR_AUTO_DUPLEX_         0x00001000
#define LAN7800_MAC_CR_AUTO_SPEED_          0x00000800
#define LAN7800_MAC_CR_LOOPBACK_            0x00000400    ///< [10] 1 = 内部ループバックを有効化
#define LAN7800_MAC_RX          0x104
#define LAN7800_MAC_RX_RXEN_                0x00000001
#define LAN7800_MAC_RX_MAX_SIZE_MASK_       0x3FFF0000
#define LAN7800_MAC_RX_MAX_SIZE_SHIFT_      16
#define LAN7800_MAC_TX          0x108
#define LAN7800_MAC_TX_TXEN_        0x00000001
#define LAN7800_FLOW            0x10C
/* MACアドレスの取得とセット */
#define LAN7800_RX_ADDRH        0x118   ///< MAC Receive Address High Register
#define LAN7800_RX_ADDRH_MASK_      0x0000FFFF
#define LAN7800_RX_ADDRL        0x11C   ///< MAC Receive Address Low Register
#define LAN7800_RX_ADDRL_MASK_      0xFFFFFFFF
/* LAN7800_MII_ACC */
#define LAN7800_MII_ACC         0x120
#define LAN7800_MII_ACC_MII_READ_   0x0
#define LAN7800_MII_ACC_MII_WRITE_  0x2
#define LAN7800_MII_ACC_MII_BUSY_   1 << 0
#define LAN7800_MII_DATA        0x124

/* USBベンダー固有リクエスト */
#define LAN7800_VENDOR_REQUEST_WRITE        0xA0
#define LAN7800_VENDOR_REQUEST_READ         0xA1
#define LAN7800_VENDOR_REQUEST_GET_STATS    0xA2

#define LAN7800_MAF_BASE        0x400         ///< MAC Address Perfect Filter Register
#define LAN7800_MAF_HIX         0x00          ///< 上位アドレス
#define LAN7800_MAF_LOX         0x04          ///< 下位アドレス
#define LAN7800_NUM_OF_MAF      33            ///< アドレスの個数
#define LAN7800_MAF_HI_BEGIN    (LAN7800_MAF_BASE + LAN7800_MAF_HIX)
#define LAN7800_MAF_LO_BEGIN    (LAN7800_MAF_BASE + LAN7800_MAF_LOX)
#define LAN7800_MAF_HI(index)   (LAN7800_MAF_BASE + (8 * (index)) + (LAN7800_MAF_HIX))
#define LAN7800_MAF_LO(index)   (LAN7800_MAF_BASE + (8 * (index)) + (LAN7800_MAF_LOX))
#define LAN7800_MAF_HI_VALID_       0x80000000    /// [31] :  アドレスはvalid

/* DMA buffer size */
#define LAN7800_ETH_MTU         1500
#define LAN7800_ETH_ALEN        6
#define LAN7800_ETH_VLAN_LEN    4

/* OTPの読み込み */
#define LAN7800_OTP_INDICATOR_1     0xF3
#define LAN7800_OTP_INDICATOR_2     0xF7
#define LAN7800_EEPROM_INDICATOR    0xA5

#define LAN7800_OTP_BASE_ADDR       0x00001000
#define LAN7800_OTP_PWR_DN          (LAN7800_OTP_BASE_ADDR + 4 * 0x00)
#define LAN7800_OTP_PWR_DN_PWRDN_N_     0x01
#define LAN7800_OTP_ADDR1           (LAN7800_OTP_BASE_ADDR + 4 * 0x01)
#define LAN7800_OTP_ADDR1_15_11         0x1F
#define LAN7800_OTP_ADDR2           (LAN7800_OTP_BASE_ADDR + 4 * 0x02)
#define LAN7800_OTP_ADDR2_10_3          0xFF
#define LAN7800_OTP_FUNC_CMD        (LAN7800_OTP_BASE_ADDR + 4 * 0x08)
#define LAN7800_OTP_FUNC_CMD_RESET_     0x04
#define LAN7800_OTP_FUNC_CMD_PROGRAM_   0x02
#define LAN7800_OTP_FUNC_CMD_READ_      0x01
#define LAN7800_OTP_CMD_GO          (LAN7800_OTP_BASE_ADDR + 4 * 0x0A)
#define LAN7800_OTP_CMD_GO_GO_          0x01
#define LAN7800_OTP_STATUS          (LAN7800_OTP_BASE_ADDR + 4 * 0x0C)
#define LAN7800_OTP_STATUS_LAN7800_OTP_LOCK_        0x10
#define LAN7800_OTP_STATUS_WEB_         0x08
#define LAN7800_OTP_STATUS_PGMEN        0x04
#define LAN7800_OTP_STATUS_CPUMPEN_     0x02
#define LAN7800_OTP_STATUS_BUSY_        0x01
#define LAN7800_OTP_RD_DATA         (LAN7800_OTP_BASE_ADDR + 4 * 0x06)

/* EEPROMの読み込み */
#define LAN7800_E2P_CMD                 0x040
#define LAN7800_E2P_CMD_EPC_BUSY_           0x80000000
#define LAN7800_E2P_CMD_EPC_ADDR_MASK_      0x000001FF
#define LAN7800_E2P_CMD_EPC_CMD_READ_       0x00000000
#define LAN7800_E2P_CMD_EPC_TIMEOUT_        0x00000400
#define LAN7800_E2P_DATA                0x044

#endif                          /* _LAN7800_H_ */
