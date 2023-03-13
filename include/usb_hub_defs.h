/**
 * @file usb_hub_defs.h
 * @ingroup usbhub
 *
 * 標準的なUSBハブ構造体と定数.
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#ifndef _USB_HUB_DEFS_H_
#define _USB_HUB_DEFS_H_

#include <usb_util.h>

#define USB_HUB_CHARACTERISTIC_IS_COMPOUND_DEVICE (1 << 2)

/** @ingroup usbhub
 * @struct usb_hub_descriptor
 * USBハブディスクリプタ（標準形式）構造体.  USB 2.0仕様のセクション11-23、表11-13を参照  */
struct usb_hub_descriptor {
    uint8_t  bDescLength;           /**< ディスクリプタ長 (バイト単位) */
    uint8_t  bDescriptorType;       /**< ディスクリプタタイプ: 0x29 = hub deescriptor */
    uint8_t  bNbrPorts;             /**< 下流方向のサポートポート数 */
    uint16_t wHubCharacteristics;   /**< 特性 [0:1], [2], [3:4], [5:6], [7]*/
    uint8_t  bPwrOn2PwrGood;        /**< 電源が安定するまでの時間 (2ms単位)*/
    uint8_t  bHubContrCurrent;      /**< 最大電流要件 (mA単位) */
    uint8_t  varData[];             /**< 可変長フィールド; 64が想定最大超のはず (255ポート => 2 x 32 バイトのデータ) */
} __packed;

/** @ingroup usbhub
 * @struct usb_port_status
 * USBポートステータス構造体. USB 2.0仕様のセクション11-24.2.7で定義されている */
struct usb_port_status {
    union {
        uint16_t wPortStatus;
        struct {
            uint16_t connected : 1;                 /**< デバイスが接続されているか */
            uint16_t enabled : 1;                   /**< ポートが有効か */
            uint16_t suspended : 1;                 /**< デバイスが停止中か */
            uint16_t overcurrent : 1;               /**< 過電流条件にあるか */
            uint16_t reset : 1;                     /**< デバイスへのリセット信号があるか */
            uint16_t wPortStatus_reserved1 : 3;     /**< 予約済み */
            uint16_t powered : 1;                   /**< 電源オンか */
            uint16_t low_speed_attached : 1;        /**< LSデバイスが接続されているか */
            uint16_t high_speed_attached : 1;       /**< HSデバイスが接続されているか */
            uint16_t test_mode : 1;                 /**< テストモードか */
            uint16_t indicator_control : 1;         /**< ポートインジケータはソフトウェア制御か */
            uint16_t wPortStatus_reserved2 : 3;     /**< 予約済み */
        };
    };
    union {
        uint16_t wPortChange;
        struct {
            uint16_t connected_changed : 1;         /**< カレント接続状態が変化したか */
            uint16_t enabled_changed : 1;           /**< 有効/無効が変化したか*/
            uint16_t suspended_changed : 1;         /**< デバイスの停止状態が変化したか*/
            uint16_t overcurrent_changed : 1;       /**< 過電流状態が変化したか*/
            uint16_t reset_changed : 1;             /**< リセット信号の有無が変化したか*/
            uint16_t wPortChange_reserved : 11;     /**< 予約済み */
        };
    };
} __packed;

/** @ingroup usbhub
 * @enum usb_port_feature
 * USBポート機能.  USB 2.0仕様のセクション11-24.2、表11-17を参照 */
enum usb_port_feature {
    USB_PORT_CONNECTION     = 0,    /**<  0 */
    USB_PORT_ENABLE         = 1,    /**<  1 */
    USB_PORT_SUSPEND        = 2,    /**<  2 */
    USB_PORT_OVER_CURRENT   = 3,    /**<  3 */
    USB_PORT_RESET          = 4,    /**<  4 */
    USB_PORT_POWER          = 8,    /**<  8 */
    USB_PORT_LOW_SPEED      = 9,    /**<  9 */
    USB_C_PORT_CONNECTION   = 16,   /**< 16 */
    USB_C_PORT_ENABLE       = 17,   /**< 17 */
    USB_C_PORT_SUSPEND      = 18,   /**< 18 */
    USB_C_PORT_OVER_CURRENT = 19,   /**< 19 */
    USB_C_PORT_RESET        = 20,   /**< 20 */
    USB_PORT_TEST           = 21,   /**< 21 */
    USB_PORT_INDICATOR      = 22,   /**< 22 */
};

/** @ingroup usbhub
 * @struct usb_hub_status
 * USBハブステータス構造体.  USB 2.0仕様のセクション11-24.2.6で定義されている */
struct usb_hub_status {
    union {
        uint16_t wHubStatus;
        struct {
            uint16_t local_power : 1;           /**< ハブパワーに電源が供給されているか */
            uint16_t overcurrent : 1;           /**< 過電流条件か */
            uint16_t wHubStatus_reserved : 14;  /**< 予約済み */
        };
    };
    union {
        uint16_t wHubChange;
        struct {
            uint16_t local_power_changed : 1;   /**< ハブパワーの状態が変化したか */
            uint16_t overcurrent_changed : 1;   /**< 過電流条件が変化したか */
            uint16_t wHubChange_reserved : 14;  /**< 予約済み */
        };
    };
} __packed;

/** @ingroup usbhub
 * @enum usb_hub_request
 * 標準USBハブリクエスト.  USB 2.0仕様のセクション11-24.2、表11-16を参照  */
enum usb_hub_request {
    USB_HUB_REQUEST_GET_STATUS       = 0,   /**< 0 */
    USB_HUB_REQUEST_CLEAR_FEATURE    = 1,   /**< 1 */
    USB_HUB_REQUEST_SET_FEATURE      = 3,   /**< 3 */
    USB_HUB_REQUEST_GET_DESCRIPTOR   = 6,   /**< 6 */
    USB_HUB_REQUEST_SET_DESCRIPTOR   = 7,   /**< 7 */
    USB_HUB_REQUEST_CLEAR_TT_BUFFER  = 8,   /**< 8 */
    USB_HUB_REQUEST_RESET_TT         = 9,   /**< 9 */
    USB_HUB_REQUEST_GET_TT_STATE     = 10,  /**< 10 */
    USB_HUB_REQUEST_STOP_TT          = 11,  /**< 11 */
};


#endif /* _USB_HUB_DEFS_H_ */
