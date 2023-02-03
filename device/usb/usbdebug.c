/**
 * @file     usbdebug.c
 * @ingroup  usbcore
 * @ingroup  usb
 *
 * このファイルにはUSBが実際に動作するために厳密には必要でない
 * USBのコア機能が含まれている。これには以下が含まれる。
 *
 * - デバッグ、情報提供、エラーメッセージの表示する。
 * - USBの状態を表示する（usbinfo()）。
 * - USBデバイスから人間が読める文字列ディスクリプタを取得する。
 * - 特定の定数をそれを説明する文字列に変換する。
 *
 * 上記の機能をすべて無効にするには usb_util.h で USB_EMBEDDED に
 * TRUE をセットする。もちろん、自分が何をしているか分かっている
 * 場合のみ、この設定を行うこと。
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <stdlib.h>
#include <stdio.h>
#include <kernel.h>
#include <interrupt.h>
#include <usb_core_driver.h>
#include <usb_hub_driver.h>
#include <usb_std_defs.h>
#include <usb_subsystem.h>

#if !USB_EMBEDDED

#if USB_MIN_LOG_PRIORITY <= USB_LOG_PRIORITY_ERROR
/**
 * エラー、情報、デバッグの各メッセージを同期的にシリアルデバイスに書き出す.
 * この関数を直接呼び出さず、usb_debg(), usb_info(), usb_error() を使用する。
 */
void usb_log(int priority, const char *func,
             struct usb_device *dev, const char *format, ...)
{
    va_list va;
    irqmask im;

    if (priority < USB_MIN_LOG_PRIORITY)
    {
        return;
    }

    im = disable();

    kprintf("USB: ");
    if (priority <= USB_LOG_PRIORITY_DEBUG)
    {
        kprintf("[DEBUG] ");
    }
    else if (priority >= USB_LOG_PRIORITY_ERROR)
    {
        kprintf("[ERROR] ");
    }
    if (dev != NULL)
    {
        kprintf("Device %u: ", dev->address);
    }
    if (priority <= USB_LOG_PRIORITY_DEBUG)
    {
        kprintf("%s(): ", func);
    }
    va_start(va, format);
    kvprintf(format, va);
    va_end(va);
    restore(im);
}
#endif

/**
 * @ingroup usbcore
 *
 * ::usb_status_t をそれを説明する文字列に変換する.
 *
 * @param status
 *      XinuのUSBサブシステムの関数から返された ::usb_status_t エラーコード
 *
 * @return
 *      エラーを説明する文字列。エラーコードが認識できない場合は "unknown error"
 */
const char *
usb_status_string(usb_status_t status)
{
    switch (status)
    {
        case USB_STATUS_SUCCESS:
            return "success";
        case USB_STATUS_OUT_OF_MEMORY:
            return "out of memory";
        case USB_STATUS_UNSUPPORTED_REQUEST:
            return "unsupported request";
        case USB_STATUS_DEVICE_UNSUPPORTED:
            return "device unsupported";
        case USB_STATUS_TIMEOUT:
            return "request timed out";
        case USB_STATUS_HARDWARE_ERROR:
            return "hardware error";
        case USB_STATUS_INVALID_DATA:
            return "invalid data";
        case USB_STATUS_INVALID_PARAMETER:
            return "invalid parameter";
        case USB_STATUS_NOT_PROCESSED:
            return "transfer not processed yet";
        case USB_STATUS_DEVICE_DETACHED:
            return "device was detached";
    }
    return "unknown error";
}

/**
 * @ingroup usbcore
 *
 * USBクラスコードをそれを説明する文字列に変換する.
 *
 * @param class_code
 *      USBクラスコード定数
 * @return
 *      USBクラスコードを説明する文字列。クラスコードが認識できない場合は "Unknown"
 */
const char *
usb_class_code_to_string(enum usb_class_code class_code)
{
    switch (class_code)
    {
        case USB_CLASS_CODE_INTERFACE_SPECIFIC:
            return "None (see interface descriptors)";
        case USB_CLASS_CODE_AUDIO:
            return "Audio";
        case USB_CLASS_CODE_COMMUNICATIONS_AND_CDC_CONTROL:
            return "Communications and CDC Control";
        case USB_CLASS_CODE_HID:
            return "HID (Human Interface Device)";
        case USB_CLASS_CODE_IMAGE:
            return "Image";
        case USB_CLASS_CODE_PRINTER:
            return "Printer";
        case USB_CLASS_CODE_MASS_STORAGE:
            return "Mass Storage";
        case USB_CLASS_CODE_HUB:
            return "Hub";
        case USB_CLASS_CODE_VIDEO:
            return "Video";
        case USB_CLASS_CODE_WIRELESS_CONTROLLER:
            return "Wireless Controller";
        case USB_CLASS_CODE_MISCELLANEOUS:
            return "Miscellaneous";
        case USB_CLASS_CODE_VENDOR_SPECIFIC:
            return "Vendor Specific";
    }
    return "Unknown";
}

/**
 * @ingroup usbcore
 *
 * USB速度定数をそれを説明する文字列に変換する.
 *
 * @param speed
 *      USB速度定数
 *
 * @return
 *      "high", "full", "low", "unknown"のいずれか。
 */
const char *
usb_speed_to_string(enum usb_speed speed)
{
    switch (speed)
    {
        case USB_SPEED_HIGH:
            return "high";
        case USB_SPEED_FULL:
            return "full";
        case USB_SPEED_LOW:
            return "low";
    }
    return "unknown";
}

/**
 * @ingroup usbcore
 *
 * USB転送タイプ定数をそれを説明する文字列に変換する.
 *
 * @param type
 *      USB転送タイプ定数
 *
 * @return
 *      "Control", "Isochronous", "Bulk", "Interrupt", "Unknown" のいずれか
 */
const char *
usb_transfer_type_to_string(enum usb_transfer_type type)
{
    switch (type)
    {
        case USB_TRANSFER_TYPE_CONTROL:
            return "Control";
        case USB_TRANSFER_TYPE_ISOCHRONOUS:
            return "Isochronous";
        case USB_TRANSFER_TYPE_BULK:
            return "Bulk";
        case USB_TRANSFER_TYPE_INTERRUPT:
            return "Interrupt";
    }
    return "Unknown";
}

/**
 * @ingroup usbcore
 *
 * USB方向定数をそれを説明する文字列に変換する.
 *
 * @param dir
 *      USB方向定数
 *
 * @return
 *      "OUT", "IN", "Unknown" のいずれか
 */
const char *
usb_direction_to_string(enum usb_direction dir)
{
    switch (dir)
    {
        case USB_DIRECTION_OUT:
            return "OUT";
        case USB_DIRECTION_IN:
            return "IN";
    }
    return "Unknown";
}


/**
 * UTF-16LEからASCIIへの遅延変換で、ASCIIでないUTF-16LEのコードポイントを
 * クエスチョンマークに置き換える.
 */
static void
utf16le_to_ascii(const uint16_t utf16le_str[], uint nchars, char *ascii_str)
{
    uint i;

    for (i = 0; i < nchars; i++)
    {
        if (utf16le_str[i] <= 0x7f)
        {
            ascii_str[i] = utf16le_str[i];
        }
        else
        {
            ascii_str[i] = '?';
        }
    }
}

/**
 * @ingroup usbcore
 *
 * デバイスからUSBストリングディスクリプタを読み込む.
 *
 * @param dev
 *      ストリングディスクリプタを読み込むデバイスのUSBデバイス構造体へのポインタ
 * @param index
 *      読み込むストリングディスクリプタのインデックス. たとえば、製品名を含む
 *      ストリングディスクリプタのインデックスを指定するデバイスディスクリプタの
 *      iProductメンバー。
 * @param lang_id
 *      リクエストする言語の言語ID. 利用可能な言語IDは任意の言語IDについて
 *      インデックス 0 のストリングディスクリプタをリクエストすることにより
 *      取得できる。
 * @param buf
 *      ストリングディスクリプタを置くバッファ
 * @param buflen
 *      読み込む最大バイト長
 *
 * @return
 *      usb_get_descriptor() を参照
 */
usb_status_t
usb_get_string_descriptor(struct usb_device *dev, uint8_t index, uint16_t lang_id,
                          struct usb_string_descriptor *buf, uint16_t buflen)
{
    return usb_get_descriptor(dev,
                              USB_DEVICE_REQUEST_GET_DESCRIPTOR,
                              USB_BMREQUESTTYPE_DIR_IN |
                                    USB_BMREQUESTTYPE_TYPE_STANDARD |
                                    USB_BMREQUESTTYPE_RECIPIENT_DEVICE,
                              (USB_DESCRIPTOR_TYPE_STRING << 8) | index, lang_id,
                              buf, buflen);
}

/**
 * @ingroup usbcore
 *
 * USBストリングディスクリプタの英語版（もしあれば）を取得し、UTF-16LEから
 * ASCIIに"変換"する. 変換後の文字列はヌル終端である。ASCIIの範囲外の
 * UTF-16LEコードポイントはクエスチョンマークに置き換えられる。
 *
 * @param dev
 *      ストリングディスクリプタを読み込むデバイスのUSBデバイス構造体へのポインタ
 * @param iString
 *      読み込むストリングディスクリプタのインデックス
 * @param strbuf
 *      ASCII文字列を置くバッファ
 * @param strbufsize
 *      @p strbuf のバッファ長
 *
 * @return
 *      usb_get_string_descriptor()が返す任意の値, 利用可能な言語IDリストが
 *      からの場合は ::USB_STATUS_INVALID_DATA
 */
usb_status_t
usb_get_ascii_string(struct usb_device *dev, uint32_t iString,
                     char *strbuf, uint32_t strbufsize)
{
    struct {
        struct usb_string_descriptor desc;
        uint16_t padding[128];
    } buf;
    uint16_t lang_id;
    usb_status_t status;
    uint i;
    uint num_languages;
    uint num_chars;

    /* 利用可能な言語リストを取得する  */
    status = usb_get_string_descriptor(dev, 0, 0, &buf.desc, sizeof(buf));
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }

    /* 利用可能な言語リストがから出ないことを確認する  */
    num_languages = (buf.desc.bLength - sizeof(struct usb_descriptor_header)) /
                        sizeof(uint16_t);
    if (num_languages == 0)
    {
        usb_dev_error(dev, "String descriptor language list is empty\n");
        return USB_STATUS_INVALID_DATA;
    }

    /* リストの最初にある英語バリアントを選択する。英語が見つからない場合は
     * リストの最初にある言語にフォールバックする。 */
    lang_id = buf.desc.bString[0];
    for (i = 0; i < num_languages; i++)
    {
        if ((buf.desc.bString[i] & USB_PRIMARY_LANGUAGE_MASK) == USB_LANG_ENGLISH)
        {
            lang_id = buf.desc.bString[i];
            break;
        }
    }

    /* 実際に必要なストリングディスクリプタを取得する  */
    status = usb_get_string_descriptor(dev, iString, lang_id,
                                       &buf.desc, sizeof(buf));
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }

    /* 文字列をUTF-16LEからASCIIに"変換"する */
    num_chars = min((buf.desc.bLength - sizeof(struct usb_descriptor_header)) /
                                sizeof(uint16_t), strbufsize - 1);
    utf16le_to_ascii(buf.desc.bString, num_chars, strbuf);
    strbuf[num_chars] = '\0';
    return USB_STATUS_SUCCESS;
}

/**
 * bcdUSB （bcd表記のUSBバージョン）を人間の読める文字列に変換する.
 *
 * @param bcdUSB
 *      変換するbcdUSB値（たとえば、USBデバイスディスクリプタから）
 *
 * @return
 *      USBバージョンを表す静的に割当られた文字列へのポインタ. この関数を
 *      次に呼び出した際にはこれは変化する。
 */
static const char *
usb_bcd_version_to_string(uint16_t bcdUSB)
{
    static char string[3 + 1 + 2 + 1 + 2 + 1];
    char *p = string;
    p += sprintf(string, "%u.%u",
                 (bcdUSB >> 8) & 0xff,  /* At most 3 digits */
                 (bcdUSB >> 4) & 0xf);  /* At most 2 digits */
                                        /* (plus period)    */
    if (bcdUSB & 0xf)
    {
        sprintf(p, ".%u", bcdUSB & 31);  /* At most 2 digits (plus period)  */
    }
    return string;
}

/**
 * @ingroup usbcore
 *
 * USBデバイスのかなり詳しい人間の読める記述.
 *
 * @param dev
 *      記述を取得するUSBデバイス
 *
 * @return
 *      デバイスを記述する文字列. 返される文字列は静的に割り当てら得たものであり、
 *      この関数を次に呼び出した際には変化する。
 */
const char *
usb_device_description(const struct usb_device *dev)
{
    uint i;
    enum usb_class_code class;
    static char device_description[512];
    char *p;

    p = device_description;

    /* Start with speed and USB version information.  */
    p += sprintf(p, "%s-speed USB %s",
                 usb_speed_to_string(dev->speed),
                 usb_bcd_version_to_string(dev->descriptor.bcdUSB));

    /* Try to find a class description of the device, taking into account that
     * the class may be stored either in the device descriptor or an interface
     * descriptor.  */
    class = dev->descriptor.bDeviceClass;
    if (class == 0)
    {
        for (i = 0; i < dev->config_descriptor->bNumInterfaces; i++)
        {
            if (dev->interfaces[i]->bInterfaceClass != 0)
            {
                class = dev->interfaces[i]->bInterfaceClass;
            }
        }
    }

    /* Add the class description if we found one and it was not something
     * meaningless like the vendor specific class.  */
    if (class != 0 &&
        class != USB_CLASS_CODE_VENDOR_SPECIFIC &&
        class != USB_CLASS_CODE_MISCELLANEOUS)
    {
        p += sprintf(p, " %s class",
                     usb_class_code_to_string(class));
    }

    /* This is indeed a device.  */
    p += sprintf(p, " device");

    /* Add the product name, if the device provides it.  */
    if (dev->product[0] != '\0')
    {
        p += sprintf(p, " (%s)", dev->product);
    }

    /* Add vendor and product IDs.  */
    p += sprintf(p, " (idVendor=0x%04x, idProduct=0x%04x)",
                 dev->descriptor.idVendor,
                 dev->descriptor.idProduct);

    /* Return the resulting string.  */
    return device_description;
}

static void
usb_print_device_descriptor(const struct usb_device *dev,
                            const struct usb_device_descriptor *desc)
{
    printf("    [Device Descriptor]\n");
    printf("    bLength:             %u\n", desc->bLength);
    printf("    bDescriptorType:     0x%02x (Device)\n", desc->bDescriptorType);
    printf("    bcdUSB:              0x%02x (USB %s compliant)\n",
           desc->bcdUSB, usb_bcd_version_to_string(desc->bcdUSB));
    printf("    bDeviceClass:        0x%02x (%s)\n",
           desc->bDeviceClass, usb_class_code_to_string(desc->bDeviceClass));
    printf("    bDeviceSubClass:     0x%02x\n", desc->bDeviceSubClass);
    printf("    bDeviceProtocol:     0x%02x\n", desc->bDeviceProtocol);
    printf("    bMaxPacketSize0:     %u\n", desc->bMaxPacketSize0);
    printf("    idVendor:            0x%04x\n", desc->idVendor);
    printf("    idProduct:           0x%04x\n", desc->idProduct);
    printf("    iManufacturer:       %u\n", desc->iManufacturer);
    if (dev->manufacturer[0] != '\0')
    {
        printf("        (%s)\n", dev->manufacturer);
    }
    printf("    iProduct:            %u\n", desc->iProduct);
    if (dev->product[0] != '\0')
    {
        printf("        (%s)\n", dev->product);
    }
    printf("    iSerialNumber:       %u\n", desc->iSerialNumber);
    printf("    bNumConfigurations:  %u\n", desc->bNumConfigurations);
    putchar('\n');
}

static void
usb_print_configuration_descriptor(const struct usb_device *dev,
                                   const struct usb_configuration_descriptor *desc)
{
    printf("        [Configuration Descriptor]\n");
    printf("        bLength:             %u\n", desc->bLength);
    printf("        bDescriptorType:     0x%02x (Configuration)\n",
           desc->bDescriptorType);
    printf("        wTotalLength:        %u\n", desc->wTotalLength);
    printf("        bNumInterfaces:      %u\n", desc->bNumInterfaces);
    printf("        bConfigurationValue: %u\n", desc->bConfigurationValue);
    printf("        iConfiguration:      %u\n", desc->iConfiguration);
    printf("        bmAttributes:        0x%02x\n", desc->bmAttributes);
    if (desc->bmAttributes & USB_CONFIGURATION_ATTRIBUTE_SELF_POWERED)
    {
        printf("            (Self powered)\n");
    }
    if (desc->bmAttributes & USB_CONFIGURATION_ATTRIBUTE_REMOTE_WAKEUP)
    {
        printf("            (Remote wakeup)\n");
    }
    printf("        bMaxPower:           %u (%u mA)\n",
           desc->bMaxPower, desc->bMaxPower * 2);
    putchar('\n');
}

static void
usb_print_interface_descriptor(const struct usb_device *dev,
                               const struct usb_interface_descriptor *desc)
{
    printf("            [Interface Descriptor]\n");
    printf("            bLength:             %u\n", desc->bLength);
    printf("            bDescriptorType:     0x%02x (Interface)\n", desc->bDescriptorType);
    printf("            bInterfaceNumber:    %u\n", desc->bInterfaceNumber);
    printf("            bAlternateSetting:   %u\n", desc->bAlternateSetting);
    printf("            bNumEndpoints:       %u\n", desc->bNumEndpoints);
    printf("            bInterfaceClass:     0x%02x (%s)\n", desc->bInterfaceClass,
           usb_class_code_to_string(desc->bInterfaceClass));
    printf("            bInterfaceSubClass:  0x%02x\n", desc->bInterfaceSubClass);
    printf("            bInterfaceProtocol:  0x%02x\n", desc->bInterfaceProtocol);
    printf("            iInterface:          %u\n", desc->iInterface);
    putchar('\n');
}

static void
usb_print_endpoint_descriptor(const struct usb_device *dev,
                              const struct usb_endpoint_descriptor *desc)
{
    printf("                [Endpoint Descriptor]\n");
    printf("                bLength:             %u\n", desc->bLength);
    printf("                bDescriptorType:     0x%02x (Endpoint)\n", desc->bDescriptorType);
    printf("                bEndpointAddress:    0x%02x (Number %u, %s)\n",
           desc->bEndpointAddress, desc->bEndpointAddress & 0xf,
           ((desc->bEndpointAddress >> 7) ? "IN" : "OUT"));
    printf("                bmAttributes:        0x%02x (%s endpoint)\n",
           desc->bmAttributes,
           ((desc->bmAttributes & 0x3) == USB_TRANSFER_TYPE_CONTROL ? "control" :
            (desc->bmAttributes & 0x3) == USB_TRANSFER_TYPE_ISOCHRONOUS ? "isochronous" :
            (desc->bmAttributes & 0x3) == USB_TRANSFER_TYPE_BULK ? "bulk" :
            "interrupt"));
    printf("                wMaxPacketSize:      0x%02x (max packet size %u bytes)\n",
           desc->wMaxPacketSize, desc->wMaxPacketSize & 0x7ff);
    printf("                bInterval:           %u\n", desc->bInterval);
    putchar('\n');
}

static void
usb_print_configuration(const struct usb_device *dev)
{
    uint i, j;

    usb_print_configuration_descriptor(dev, dev->config_descriptor);
    for (i = 0; i < dev->config_descriptor->bNumInterfaces; i++)
    {
        usb_print_interface_descriptor(dev, dev->interfaces[i]);
        for (j = 0; j < dev->interfaces[i]->bNumEndpoints; j++)
        {
            usb_print_endpoint_descriptor(dev, dev->endpoints[i][j]);
        }
    }
}

/**
 * USBデバイスに関する情報を表示する.
 */
static usb_status_t
usbinfo_device_callback(struct usb_device *dev)
{
    printf("[USB Device %03u]\n", dev->address);
    usb_print_device_descriptor(dev, &dev->descriptor);
    usb_print_configuration(dev);

    putchar('\n');
    return USB_STATUS_SUCCESS;
}

#define USBINFO_TREE_SPACES_PER_LEVEL  6
#define USBINFO_TREE_LINES_PER_PORT    2

static usb_status_t
usbinfo_tree_callback(struct usb_device *dev)
{
    uint i, j, n;

    if (dev->depth != 0)
    {
        n = (dev->depth - 1) * USBINFO_TREE_SPACES_PER_LEVEL;
        for (j = 0; j < USBINFO_TREE_LINES_PER_PORT; j++)
        {
            for (i = 0; i < n; i++)
            {
                putchar(' ');
            }
            putchar('|');
            putchar('\n');
        }
        for (i = 0; i < n; i++)
        {
            putchar(' ');
        }
        for (i = 0; i < USBINFO_TREE_SPACES_PER_LEVEL; i++)
        {
            putchar('-');
        }
    }
    printf("%03u [%s]\n", dev->address, usb_device_description(dev));
    return USB_STATUS_SUCCESS;
}

#endif /* !USB_EMBEDDED */

/**
 * @ingroup usb
 *
 * USBに接続されているすべてのデバイスに関する情報を表示する.
 * これは printf() を使用するので、（シェルなどから）割り込みを
 * 有効にして呼び出す必要がある。
 *
 * @return
 *      USBサブシステムが初期化されていない場合は ::SYSERR; そうでなければ ::OK
 */
syscall usbinfo(void)
{
#if USB_EMBEDDED
    fprintf(stderr, "usbinfo not supported in USB_EMBEDDED mode.\n");
    return SYSERR;
#else
    struct usb_device *root_hub = usb_root_hub;

    if (root_hub == NULL)
    {
        fprintf(stderr, "USB subsystem not initialized.\n");
        return SYSERR;
    }

    /* USBは動的なバスであり、デバイスはいつでも着脱される可能性がある。
     * そのため、デバイスがバスから着脱されるのを防ぐために、バスを
     * 「ロック」する必要がある。そうしないとバスの正しいスナップショットを
     * 得られない可能性があり、表示中にデバイスが解放される可能性すらある */
    usb_lock_bus();
    usb_hub_for_device_in_tree(root_hub, usbinfo_device_callback);

    printf("\nDiagram of USB:\n\n");
    usb_hub_for_device_in_tree(root_hub, usbinfo_tree_callback);
    usb_unlock_bus();
    return OK;
#endif /* !USB_EMBEDDED */
}
