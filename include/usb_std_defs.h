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

/** USB転送の方向（ホスト相対で定義されている） */
enum usb_direction {
    USB_DIRECTION_OUT = 0,  /* ホストからデバイス */
    USB_DIRECTION_IN = 1    /* デバイスからホスト */
};

/** 転送速度 */
enum usb_speed {
    USB_SPEED_HIGH = 0,
    USB_SPEED_FULL = 1,
    USB_SPEED_LOW  = 2,
};

/** 転送タイプ */
enum usb_transfer_type {
    USB_TRANSFER_TYPE_CONTROL     = 0,
    USB_TRANSFER_TYPE_ISOCHRONOUS = 1,
    USB_TRANSFER_TYPE_BULK        = 2,
    USB_TRANSFER_TYPE_INTERRUPT   = 3,
};

/** 転送サイズ */
enum usb_transfer_size {
    USB_TRANSFER_SIZE_8_BIT  = 0,
    USB_TRANSFER_SIZE_16_BIT = 1,
    USB_TRANSFER_SIZE_32_BIT = 2,
    USB_TRANSFER_SIZE_64_BIT = 3,
};

/** 標準USBディスクリプトタイプ. USB 2.0仕様のセクション 9.4の表 9-5 を参照 */
enum usb_descriptor_type {
    USB_DESCRIPTOR_TYPE_DEVICE        = 1,
    USB_DESCRIPTOR_TYPE_CONFIGURATION = 2,
    USB_DESCRIPTOR_TYPE_STRING        = 3,
    USB_DESCRIPTOR_TYPE_INTERFACE     = 4,
    USB_DESCRIPTOR_TYPE_ENDPOINT      = 5,
    USB_DESCRIPTOR_TYPE_HUB           = 0x29,
};

/** USBリクエストタイプ（bmRequestTypeのビット 6..5）. USB 2.0仕様のセクション 9.3の表 9-2 を参照 */
enum usb_request_type {
    USB_REQUEST_TYPE_STANDARD = 0,
    USB_REQUEST_TYPE_CLASS    = 1,
    USB_REQUEST_TYPE_VENDOR   = 2,
    USB_REQUEST_TYPE_RESERVED = 3,
};

/** USBリクエスト受信者（bmRequestTypeのビット  4..0）. USB 2.0仕様のセクション 9.3の表 9-2 を参照 */
enum usb_request_recipient {
    USB_REQUEST_RECIPIENT_DEVICE    = 0,
    USB_REQUEST_RECIPIENT_INTERFACE = 1,
    USB_REQUEST_RECIPIENT_ENDPOINT  = 2,
    USB_REQUEST_RECIPIENT_OTHER     = 3,
};

/** SETUPデータの bmRequestTypeメンバー内のビットフィールド値.
 * USB 2.0仕様のセクション 9.3の表 9-2 を参照 */
enum usb_bmRequestType_fields {
    USB_BMREQUESTTYPE_DIR_OUT             = (USB_DIRECTION_OUT << 7),
    USB_BMREQUESTTYPE_DIR_IN              = (USB_DIRECTION_IN << 7),
    USB_BMREQUESTTYPE_DIR_MASK            = (0x1 << 7),
    USB_BMREQUESTTYPE_TYPE_STANDARD       = (USB_REQUEST_TYPE_STANDARD << 5),
    USB_BMREQUESTTYPE_TYPE_CLASS          = (USB_REQUEST_TYPE_CLASS << 5),
    USB_BMREQUESTTYPE_TYPE_VENDOR         = (USB_REQUEST_TYPE_VENDOR << 5),
    USB_BMREQUESTTYPE_TYPE_RESERVED       = (USB_REQUEST_TYPE_RESERVED << 5),
    USB_BMREQUESTTYPE_TYPE_MASK           = (0x3 << 5),
    USB_BMREQUESTTYPE_RECIPIENT_DEVICE    = (USB_REQUEST_RECIPIENT_DEVICE << 0),
    USB_BMREQUESTTYPE_RECIPIENT_INTERFACE = (USB_REQUEST_RECIPIENT_INTERFACE << 0),
    USB_BMREQUESTTYPE_RECIPIENT_ENDPOINT  = (USB_REQUEST_RECIPIENT_ENDPOINT << 0),
    USB_BMREQUESTTYPE_RECIPIENT_OTHER     = (USB_REQUEST_RECIPIENT_OTHER << 0),
    USB_BMREQUESTTYPE_RECIPIENT_MASK      = (0x1f << 0),
};

/** 標準USBデバイスリクエスト. USB 2.0仕様のセクション 9.4の表 9-3 を参照 */
enum usb_device_request {
    USB_DEVICE_REQUEST_GET_STATUS        = 0,
    USB_DEVICE_REQUEST_CLEAR_FEATURE     = 1,
    USB_DEVICE_REQUEST_SET_FEATURE       = 3,
    USB_DEVICE_REQUEST_SET_ADDRESS       = 5,
    USB_DEVICE_REQUEST_GET_DESCRIPTOR    = 6,
    USB_DEVICE_REQUEST_SET_DESCRIPTOR    = 7,
    USB_DEVICE_REQUEST_GET_CONFIGURATION = 8,
    USB_DEVICE_REQUEST_SET_CONFIGURATION = 9,
    USB_DEVICE_REQUEST_GET_INTERFACE     = 10,
    USB_DEVICE_REQUEST_SET_INTERFACE     = 11,
    USB_DEVICE_REQUEST_SYNCH_FRAME       = 12,
};

/** 一般に使われているUSBクラスコード. USB 2.0仕様で定義されているのは
 * ハブクラスだけであることに注意されたい。他の標準クラスコードは
 * 別の仕様で定義されている。  */
enum usb_class_code {
    USB_CLASS_CODE_INTERFACE_SPECIFIC             = 0x00,
    USB_CLASS_CODE_AUDIO                          = 0x01,
    USB_CLASS_CODE_COMMUNICATIONS_AND_CDC_CONTROL = 0x02,
    USB_CLASS_CODE_HID                            = 0x03,
    USB_CLASS_CODE_IMAGE                          = 0x06,
    USB_CLASS_CODE_PRINTER                        = 0x07,
    USB_CLASS_CODE_MASS_STORAGE                   = 0x08,
    USB_CLASS_CODE_HUB                            = 0x09,
    USB_CLASS_CODE_VIDEO                          = 0x0e,
    USB_CLASS_CODE_WIRELESS_CONTROLLER            = 0xe0,
    USB_CLASS_CODE_MISCELLANEOUS                  = 0xef,
    USB_CLASS_CODE_VENDOR_SPECIFIC                = 0xff,
};

/** 標準USB主言語IDの超短縮リスト. USB 2.0仕様の言語識別子補遺で定義されている。
 * 16ビットの言語識別子の下位10ビットである。  */
enum usb_primary_language_id {
    USB_LANG_ENGLISH = 0x09,
};

#define USB_PRIMARY_LANGUAGE_MASK 0x3ff

/** 標準USB副言語IDの超短縮リスト. USB 2.0仕様の言語識別子補遺で定義されている。
 * 16ビットの言語識別子の上位6ビットである。  */
enum usb_sublanguage_id {
    USB_SUBLANG_ENGLISH_US = 0x01,
};

#define USB_LANGID_US_ENGLISH \
            (USB_LANG_ENGLISH | (USB_SUBLANG_ENGLISH_US << 10))

/** USBコントロールリクエスト用の標準SETUPデータ. USB 2.0仕様のセクション 9.3の表 9-2 を参照 */
struct usb_control_setup_data {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __packed;

/** すべての標準USBディクリプタの戦闘フィールド  */
struct usb_descriptor_header {
    uint8_t bLength;
    uint8_t bDescriptorType;
} __packed;

/** USBデバイスディスクリプタの標準フォーマット. USB 2.0仕様の9.6.3の表 9-8 を参照 */
struct usb_device_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __packed;

/** USBコンフィグレーションディスクリプタの標準フォーマット. USB 2.0仕様の9.6.3の表 9-18 を参照  */
struct usb_configuration_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;
} __packed;

/* usb_configuration_descriptor構造体のbmAttributesの値 */
#define USB_CONFIGURATION_ATTRIBUTE_RESERVED_HIGH   0x80
#define USB_CONFIGURATION_ATTRIBUTE_SELF_POWERED    0x40
#define USB_CONFIGURATION_ATTRIBUTE_REMOTE_WAKEUP   0x20
#define USB_CONFIGURATION_ATTRIBUTE_RESERVED_LOW    0x1f

/** USBインタフェースディスクリプタの標準フォーマット. USB 2.0仕様の9.6.6の表 9-12 を参照 */
struct usb_interface_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __packed;

/** USBエンドポイントディスクリプタの標準フォーマット. USB 2.0仕様の9.6.6の表 9-13 を参照  */
struct usb_endpoint_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __packed;

/** USBストリングディスクリプタの標準フォーマット. USB 2.0仕様のセクション9.7の表 9-16 を参照 */
struct usb_string_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bString[]; /* UTF-16LE encoded string
                      (spec ambigiously just says "UNICODE") */
} __packed;

#define USB_DEVICE_STATUS_SELF_POWERED  (1 << 0)
#define USB_DEVICE_STATUS_REMOTE_WAKEUP (1 << 1)

/** usb_device_request::USB_DEVICE_REQUEST_GET_STATUSコントロール
 * メッセージにより返されるデバイスステータス情報のフォーマット.
 * USB 2.0仕様のセクション9.4.6に記載 */
struct usb_device_status {
    uint16_t wStatus;
} __packed;

/**
 * ミリ秒あたりのUSBフレーム数.
 */
#define USB_FRAMES_PER_MS  1

/**
 * ミリ秒あたりのUSBマイクロフレーム数.
 */
#define USB_UFRAMES_PER_MS 8

#endif /* _USB_STD_DEFS_H_ */
