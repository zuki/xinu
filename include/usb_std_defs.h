/**
 * @file usb_std_defs.h
 * @ingroup usbcore
 *
 * USB 2.0 仕様で定義されている構造体と定数.
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#ifndef _USB_STD_DEFS_H_
#define _USB_STD_DEFS_H_

#include <usb_util.h>

/** @ingroup usbcore
 *  @enum  usb_direction
 * USB転送の方向（ホスト相対で定義されている） */
enum usb_direction {
    USB_DIRECTION_OUT = 0,  /**< 0: ホストからデバイス */
    USB_DIRECTION_IN = 1    /**< 1: デバイスからホスト */
};

/** @ingroup usbcore
 *  @enum  usb_speed
 * 転送速度 */
enum usb_speed {
    USB_SPEED_HIGH = 0,     /**< 0 */
    USB_SPEED_FULL = 1,     /**< 1 */
    USB_SPEED_LOW  = 2,     /**< 2 */
};

/** @ingroup usbcore
 *  @enum  usb_transfer_type
 * 転送タイプ */
enum usb_transfer_type {
    USB_TRANSFER_TYPE_CONTROL     = 0,      /**< 0 */
    USB_TRANSFER_TYPE_ISOCHRONOUS = 1,      /**< 1 */
    USB_TRANSFER_TYPE_BULK        = 2,      /**< 2 */
    USB_TRANSFER_TYPE_INTERRUPT   = 3,      /**< 3 */
};

/** @ingroup usbcore
 *  @enum  usb_transfer_size
 * 転送サイズ */
enum usb_transfer_size {
    USB_TRANSFER_SIZE_8_BIT  = 0,       /**< 0 */
    USB_TRANSFER_SIZE_16_BIT = 1,       /**< 1 */
    USB_TRANSFER_SIZE_32_BIT = 2,       /**< 2 */
    USB_TRANSFER_SIZE_64_BIT = 3,       /**< 3 */
};

/** @ingroup usbcore
 *  @enum  usb_descriptor_type
 * 標準USBディスクリプトタイプ. USB 2.0仕様のセクション 9.4の表 9-5 を参照 */
enum usb_descriptor_type {
    USB_DESCRIPTOR_TYPE_DEVICE        = 1,      /**< 1 */
    USB_DESCRIPTOR_TYPE_CONFIGURATION = 2,      /**< 2 */
    USB_DESCRIPTOR_TYPE_STRING        = 3,      /**< 3 */
    USB_DESCRIPTOR_TYPE_INTERFACE     = 4,      /**< 4 */
    USB_DESCRIPTOR_TYPE_ENDPOINT      = 5,      /**< 5 */
    USB_DESCRIPTOR_TYPE_HUB           = 0x29,   /**< 0x29 */
};

/** @ingroup usbcore
 *  @enum  usb_request_type
 * USBリクエストタイプ（bmRequestTypeのビット 6..5）. USB 2.0仕様のセクション 9.3の表 9-2 を参照 */
enum usb_request_type {
    USB_REQUEST_TYPE_STANDARD = 0,      /**< 0 */
    USB_REQUEST_TYPE_CLASS    = 1,      /**< 1 */
    USB_REQUEST_TYPE_VENDOR   = 2,      /**< 2 */
    USB_REQUEST_TYPE_RESERVED = 3,      /**< 3 */
};

/** @ingroup usbcore
 *  @enum  usb_request_recipient
 * USBリクエスト受信者（bmRequestTypeのビット  4..0）. USB 2.0仕様のセクション 9.3の表 9-2 を参照 */
enum usb_request_recipient {
    USB_REQUEST_RECIPIENT_DEVICE    = 0,        /**< 0 */
    USB_REQUEST_RECIPIENT_INTERFACE = 1,        /**< 1 */
    USB_REQUEST_RECIPIENT_ENDPOINT  = 2,        /**< 2 */
    USB_REQUEST_RECIPIENT_OTHER     = 3,        /**< 3 */
};

/** @ingroup usbcore
 *  @enum  usb_bmRequestType_fields
 * SETUPデータの bmRequestTypeメンバー内のビットフィールド値.
 * USB 2.0仕様のセクション 9.3の表 9-2 を参照 */
enum usb_bmRequestType_fields {
    USB_BMREQUESTTYPE_DIR_OUT             = (USB_DIRECTION_OUT << 7),                   /**< 0 << 7 */
    USB_BMREQUESTTYPE_DIR_IN              = (USB_DIRECTION_IN << 7),                    /**< 1 << 7 */
    USB_BMREQUESTTYPE_DIR_MASK            = (0x1 << 7),                                 /**< 0x1 << 7 */
    USB_BMREQUESTTYPE_TYPE_STANDARD       = (USB_REQUEST_TYPE_STANDARD << 5),           /**< 0 << 5 */
    USB_BMREQUESTTYPE_TYPE_CLASS          = (USB_REQUEST_TYPE_CLASS << 5),              /**< 1 << 5 */
    USB_BMREQUESTTYPE_TYPE_VENDOR         = (USB_REQUEST_TYPE_VENDOR << 5),             /**< 2 << 5*/
    USB_BMREQUESTTYPE_TYPE_RESERVED       = (USB_REQUEST_TYPE_RESERVED << 5),           /**< 3 << 5 */
    USB_BMREQUESTTYPE_TYPE_MASK           = (0x3 << 5),                                 /**< 0x3 << 5 */
    USB_BMREQUESTTYPE_RECIPIENT_DEVICE    = (USB_REQUEST_RECIPIENT_DEVICE << 0),        /**< 0 << 0 */
    USB_BMREQUESTTYPE_RECIPIENT_INTERFACE = (USB_REQUEST_RECIPIENT_INTERFACE << 0),     /**< 1 << 0 */
    USB_BMREQUESTTYPE_RECIPIENT_ENDPOINT  = (USB_REQUEST_RECIPIENT_ENDPOINT << 0),      /**< 2 << 0 */
    USB_BMREQUESTTYPE_RECIPIENT_OTHER     = (USB_REQUEST_RECIPIENT_OTHER << 0),         /**< 3 << 0 */
    USB_BMREQUESTTYPE_RECIPIENT_MASK      = (0x1f << 0),                                /**< 0x1F << 0 */
};

/** @ingroup usbcore
 *  @enum  usb_device_request
 * 標準USBデバイスリクエスト. USB 2.0仕様のセクション 9.4の表 9-3 を参照 */
enum usb_device_request {
    USB_DEVICE_REQUEST_GET_STATUS        = 0,       /**< 0 */
    USB_DEVICE_REQUEST_CLEAR_FEATURE     = 1,       /**< 1 */
    USB_DEVICE_REQUEST_SET_FEATURE       = 3,       /**< 3 */
    USB_DEVICE_REQUEST_SET_ADDRESS       = 5,       /**< 5 */
    USB_DEVICE_REQUEST_GET_DESCRIPTOR    = 6,       /**< 6 */
    USB_DEVICE_REQUEST_SET_DESCRIPTOR    = 7,       /**< 7 */
    USB_DEVICE_REQUEST_GET_CONFIGURATION = 8,       /**< 8 */
    USB_DEVICE_REQUEST_SET_CONFIGURATION = 9,       /**< 9 */
    USB_DEVICE_REQUEST_GET_INTERFACE     = 10,      /**< 10 */
    USB_DEVICE_REQUEST_SET_INTERFACE     = 11,      /**< 11 */
    USB_DEVICE_REQUEST_SYNCH_FRAME       = 12,      /**< 12 */
};

/** @ingroup usbcore
 *  @enum  usb_class_code
 * 一般に使われているUSBクラスコード. USB 2.0仕様で定義されているのは
 * ハブクラスだけであることに注意されたい。他の標準クラスコードは
 * 別の仕様で定義されている。  */
enum usb_class_code {
    USB_CLASS_CODE_INTERFACE_SPECIFIC             = 0x00,       /**< 0x00 */
    USB_CLASS_CODE_AUDIO                          = 0x01,       /**< 0x01 */
    USB_CLASS_CODE_COMMUNICATIONS_AND_CDC_CONTROL = 0x02,       /**< 0x02 */
    USB_CLASS_CODE_HID                            = 0x03,       /**< 0x03 */
    USB_CLASS_CODE_IMAGE                          = 0x06,       /**< 0x06 */
    USB_CLASS_CODE_PRINTER                        = 0x07,       /**< 0x07 */
    USB_CLASS_CODE_MASS_STORAGE                   = 0x08,       /**< 0x08 */
    USB_CLASS_CODE_HUB                            = 0x09,       /**< 0x09 */
    USB_CLASS_CODE_VIDEO                          = 0x0e,       /**< 0x0E */
    USB_CLASS_CODE_WIRELESS_CONTROLLER            = 0xe0,       /**< 0xE0 */
    USB_CLASS_CODE_MISCELLANEOUS                  = 0xef,       /**< 0xEF */
    USB_CLASS_CODE_VENDOR_SPECIFIC                = 0xff,       /**< 0xFF */
};

/** @ingroup usbcore
 *  @enum  usb_primary_language_id
 * 標準USB主言語IDの超短縮リスト. USB 2.0仕様の言語識別子補遺で定義されている。
 * 16ビットの言語識別子の下位10ビットである。  */
enum usb_primary_language_id {
    USB_LANG_ENGLISH = 0x09,        /**< 0x09 */
};

/**  @ingroup usbcore
 * @def USB_PRIMARY_LANGUAGE_MASK
 * USB主言語を取り出すためのマスク値.
*/
#define USB_PRIMARY_LANGUAGE_MASK 0x3ff

/** @ingroup usbcore
 *  @enum  usb_sublanguage_id
 * 標準USB副言語IDの超短縮リスト. USB 2.0仕様の言語識別子補遺で定義されている。
 * 16ビットの言語識別子の上位6ビットである。  */
enum usb_sublanguage_id {
    USB_SUBLANG_ENGLISH_US = 0x01,      /**< 0x01 */
};

/**  @ingroup usbcore
 * @def USB_LANGID_US_ENGLISH
 * アメリカ英語のUSB言語コード値.
*/
#define USB_LANGID_US_ENGLISH \
            (USB_LANG_ENGLISH | (USB_SUBLANG_ENGLISH_US << 10))

/** @ingroup usbcore
 * @struct usb_control_setup_data
 * USBコントロールリクエスト用の標準SETUPデータ構造体. USB 2.0仕様のセクション 9.3の表 9-2 を参照 */
struct usb_control_setup_data {
    uint8_t  bmRequestType;     /**< リクエストタイプ */
    uint8_t  bRequest;          /**< リクエストコード */
    uint16_t wValue;            /**< リクエスト依存のデータ */
    uint16_t wIndex;            /**< リクエスト依存であるが主にインデックスまたは王セット */
    uint16_t wLength;           /**< データステージに転送するバイト長 */
} __packed;

/** @ingroup usbcore
 * @struct usb_descriptor_header
 * すべての標準USBディクリプタの先頭フィールド */
struct __attribute__((__packed__, aligned(4))) usb_descriptor_header {
    uint8_t bLength;            /**< ディスクリプト長 (バイト単位) */
    uint8_t bDescriptorType;    /**< ディスクリプトタイプ */
};

/** @ingroup usbcore
 * @struct usb_device_descriptor
 * USBデバイスディスクリプタの標準フォーマット. USB 2.0仕様の9.6.3の表 9-8 を参照 */
struct usb_device_descriptor {
    uint8_t  bLength;               /**< ディスクリプト長 (バイト単位) */
    uint8_t  bDescriptorType;       /**< ディスクリプトタイプ */
    uint16_t bcdUSB;                /**< USB使用リリース番号 */
    uint8_t  bDeviceClass;          /**< デバイスクラスコード */
    uint8_t  bDeviceSubClass;       /**< デバイスサブクラスコード */
    uint8_t  bDeviceProtocol;       /**< デバイスプロトコルコード */
    uint8_t  bMaxPacketSize0;       /**< エンドポイント0の最大パケットサイズ */
    uint16_t idVendor;              /**< 製造者ID */
    uint16_t idProduct;             /**< 製品ID */
    uint16_t bcdDevice;             /**< デバイスリリース番号 */
    uint8_t  iManufacturer;         /**< 製造者を表す文字列ディスクリプタのインデックス */
    uint8_t  iProduct;              /**< 製品を表す文字列ディスクリプタのインデックス */
    uint8_t  iSerialNumber;         /**< デバイスシリアル番号を表す文字列ディスクリプタのインデックス */
    uint8_t  bNumConfigurations;    /**< コンフィグレーションの数 */
} __packed;

/** @ingroup usbcore
 * @struct usb_configuration_descriptor
 * USBコンフィグレーションディスクリプタの標準フォーマット. USB 2.0仕様の9.6.3の表 9-18 を参照  */
struct usb_configuration_descriptor {
    uint8_t  bLength;                   /**< ディスクリプト長 (バイト単位) */
    uint8_t  bDescriptorType;           /**< ディスクリプトタイプ */
    uint16_t wTotalLength;              /**< トータルデータ長 */
    uint8_t  bNumInterfaces;            /**< サポートするインタフェースの数*/
    uint8_t  bConfigurationValue;       /**< このコンフィグレーションを選択するために使用する値 */
    uint8_t  iConfiguration;            /**< このコンフィグレーションを表す文字列ディスクリプタのインデックス*/
    uint8_t  bmAttributes;              /**< コンフィグレーション属性 */
    uint8_t  bMaxPower;                 /**< 最大電力消費量 */
} __packed;

/* usb_configuration_descriptor構造体のbmAttributesの値 */
/** @ingroup usbcore
 * @def USB_CONFIGURATION_ATTRIBUTE_RESERVED_HIGH
 * コンフィグレーション属性のbit7の予約マスク値 */
#define USB_CONFIGURATION_ATTRIBUTE_RESERVED_HIGH   0x80
/** @ingroup usbcore
 * @def USB_CONFIGURATION_ATTRIBUTE_SELF_POWERED
 * コンフィグレーション属性のセルフパワー指定値 */
#define USB_CONFIGURATION_ATTRIBUTE_SELF_POWERED    0x40
/** @ingroup usbcore
 * @def USB_CONFIGURATION_ATTRIBUTE_REMOTE_WAKEUP
 * コンフィグレーション属性のリモートウェイクアップ指定値 */
#define USB_CONFIGURATION_ATTRIBUTE_REMOTE_WAKEUP   0x20
/** @ingroup usbcore
 * @def USB_CONFIGURATION_ATTRIBUTE_RESERVED_LOW
 * コンフィグレーション属性のbit[4:0]の予約マスク値 */
#define USB_CONFIGURATION_ATTRIBUTE_RESERVED_LOW    0x1f

/** @ingroup usbcore
 * @struct usb_interface_descriptor
 * USBインタフェースディスクリプタの標準フォーマット. USB 2.0仕様の9.6.6の表 9-12 を参照 */
struct usb_interface_descriptor {
    uint8_t bLength;                /**< ディスクリプト長 (バイト単位) */
    uint8_t bDescriptorType;        /**< ディスクリプトタイプ */
    uint8_t bInterfaceNumber;       /**< インタフェース番号 */
    uint8_t bAlternateSetting;      /**< この代替インタフェースを選択するための値 */
    uint8_t bNumEndpoints;          /**< 使用するエンドポイントの数 */
    uint8_t bInterfaceClass;        /**< クラスコード */
    uint8_t bInterfaceSubClass;     /**< サブクラスコード */
    uint8_t bInterfaceProtocol;     /**< プロトコルコード */
    uint8_t iInterface;             /**< このインタフェースを表す文字列ディスクリプタのインデックス */
} __packed;

/** @ingroup usbcore
 * @struct usb_endpoint_descriptor
 * USBエンドポイントディスクリプタの標準フォーマット. USB 2.0仕様の9.6.6の表 9-13 を参照  */
struct usb_endpoint_descriptor {
    uint8_t bLength;                /**< ディスクリプト長 (バイト単位) */
    uint8_t bDescriptorType;        /**< ディスクリプトタイプ */
    uint8_t bEndpointAddress;       /**< エンドポイントアドレス */
    uint8_t bmAttributes;           /**< エンドポイント属性 */
    uint16_t wMaxPacketSize;        /**< 最大パケットサイズ */
    uint8_t bInterval;              /**< データ転送のためにエンドポイントをポーリングする間隔 */
} __packed;

/** @ingroup usbcore
 * @struct usb_string_descriptor
 * USBストリングディスクリプタの標準フォーマット. USB 2.0仕様のセクション9.7の表 9-16 を参照 */
struct usb_string_descriptor {
    uint8_t  bLength;           /**< ディスクリプト長 (バイト単位) */
    uint8_t  bDescriptorType;   /**< ディスクリプトタイプ */
    uint16_t bString[];         /**< UTF-16LEでエンコードされた文字列encoded string
                                    (使用では単に"UNICODE"と書かれている) */
} __packed;

/** @ingroup usbcore
 * @def USB_DEVICE_STATUS_SELF_POWERED
 * デバイスステータスにおけるセルフパワービットの位置
 */
#define USB_DEVICE_STATUS_SELF_POWERED  (1 << 0)
/** @ingroup usbcore
 * @def USB_DEVICE_STATUS_REMOTE_WAKEUP
 * デバイスステータスにおけるリモートウェイクアップビットの位置
 */
#define USB_DEVICE_STATUS_REMOTE_WAKEUP (1 << 1)

/** @ingroup usbcore
 * @struct usb_device_status
 * usb_device_request::USB_DEVICE_REQUEST_GET_STATUS コントロール
 * メッセージにより返されるデバイスステータス情報のフォーマット.
 * USB 2.0仕様のセクション9.4.6に記載 */
struct usb_device_status {
    uint16_t wStatus;
} __packed;

/** @ingroup usbcore
 * @def USB_FRAMES_PER_MS
 * ミリ秒あたりのUSBフレーム数.
 */
#define USB_FRAMES_PER_MS  1

/** @ingroup usbcore
 * @def USB_UFRAMES_PER_MS
 * ミリ秒あたりのUSBマイクロフレーム数.
 */
#define USB_UFRAMES_PER_MS 8

#endif /* _USB_STD_DEFS_H_ */
