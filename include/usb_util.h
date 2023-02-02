/**
 * @file usb_util.h
 * @ingroup usbcore
 *
 * USBコアで使用される様々な定義.
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#ifndef _USB_UTIL_H_
#define _USB_UTIL_H_

#include <kernel.h>
#include <stddef.h>
#include <stdint.h>

struct usb_device;

/**********************************************************************/

/* コンフィグレーション変数（他のファイルに移動したほうがよいかも） */

/** USB "embedded" モードを有効にする: TRUE をセットすると、USB
 * デバッグ、情報、エラーメッセージは表示されず、'usbinfo' シェル
 * コマンドも使用できなくなる。コンパイル済のコードサイズを大幅に削減
 * する  */
#define USB_EMBEDDED                FALSE

/** USBメッセージの最小優先度. これ以上の優先度を持つメッセージ
 * だけが表示される */
#define USB_MIN_LOG_PRIORITY        4

/**********************************************************************/

/** USBエラーメッセージの優先度.  */
#define USB_LOG_PRIORITY_ERROR      3

/** USB情報メッセージ（デバイの着脱など）の優先度. */
#define USB_LOG_PRIORITY_INFO       2

/** USBデバッグメッセージの優先度.  */
#define USB_LOG_PRIORITY_DEBUG      1

#if USB_EMBEDDED && USB_MIN_LOG_PRIORITY <= USB_LOG_PRIORITY_ERROR
#  undef USB_MIN_LOG_PRIORITY
#  define USB_MIN_LOG_PRIORITY USB_LOG_PRIORITY_ERROR + 1
#endif

#if USB_LOG_PRIORITY_ERROR >= USB_MIN_LOG_PRIORITY
void usb_log(int priority, const char *func,
             struct usb_device *dev, const char *format, ...)
                __printf_format(4, 5);
#endif

#if USB_LOG_PRIORITY_ERROR >= USB_MIN_LOG_PRIORITY
#  define usb_dev_error(dev, format, ...) \
        usb_log(USB_LOG_PRIORITY_ERROR, __func__, dev, format, ##__VA_ARGS__)
#else
#  define usb_dev_error(dev, format, ...)
#endif

#if USB_LOG_PRIORITY_INFO >= USB_MIN_LOG_PRIORITY
#  define usb_dev_info(dev, format, ...) \
        usb_log(USB_LOG_PRIORITY_INFO, __func__, dev, format, ##__VA_ARGS__)
#else
#  define usb_dev_info(dev, format, ...)
#endif

#if USB_LOG_PRIORITY_DEBUG >= USB_MIN_LOG_PRIORITY
#  define usb_dev_debug(dev, format, ...) \
        usb_log(USB_LOG_PRIORITY_DEBUG, __func__, dev, format, ##__VA_ARGS__)
#else
#  define usb_dev_debug(dev, format, ...)
#endif

#define usb_error(format, ...) usb_dev_error(NULL, format, ##__VA_ARGS__)
#define usb_info(format, ...)  usb_dev_info (NULL, format, ##__VA_ARGS__)
#define usb_debug(format, ...) usb_dev_debug(NULL, format, ##__VA_ARGS__)

/** USBサブシステムの多くの関数が返すステータスコード. 汎用的なXinu SYSERRでは
 * 十分な情報が得られない場合が数多くある。  */
typedef enum usb_status {

    /** 関数は成功した.  */
    USB_STATUS_SUCCESS                   =  0,

    /** USBデバイスが取り外された.  */
    USB_STATUS_DEVICE_DETACHED           = -1,

    /** このUSBデバイスはドライバでサポートされていない.  */
    USB_STATUS_DEVICE_UNSUPPORTED        = -2,

    /** 何らかのハードウェアエラーが発生した.  */
    USB_STATUS_HARDWARE_ERROR            = -3,

    /** 不正なデータを受信した.  */
    USB_STATUS_INVALID_DATA              = -4,

    /** 関数に不正な引数が渡された.  */
    USB_STATUS_INVALID_PARAMETER         = -5,

    /** USB転送はまだ処理されていない  */
    USB_STATUS_NOT_PROCESSED             = -6,

    /** 必要なメモリの割り当てに失敗した.  */
    USB_STATUS_OUT_OF_MEMORY             = -7,

    /** 操作がタイム・アウトした.  */
    USB_STATUS_TIMEOUT                   = -8,

    /** このリクエストはサポートされていない.  */
    USB_STATUS_UNSUPPORTED_REQUEST       = -9,
} usb_status_t;

#if !USB_EMBEDDED
const char *usb_status_string(usb_status_t status);
#endif

#endif /* _USB_UTIL_H_ */
