/**
 * @file     usbhub.c
 * @ingroup  usbhub
 * @ingroup  usb
 *
 * このファイルにはUSBハブデバイスドライバが含まれている.
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

/**
 * @addtogroup usbhub
 *
 * USBハブドライバである。ハブはUSBの基本的なデバイスの1つであり、デバイスを追加
 * するための接続ポイント（ポート）を提供するために使用される。「外部」ハブが接続
 * されていないとしても、USBには少なくとも1つ論理ハブ（ルートハブ）があり、通常は、
 * さらに「内部」ハブも存在することに注意されたい。すわわち、USBはデバイスのツリーで
 * あり、ルートノードと非リーフのすべてのノードがハブである。USBハブのポートは
 * デバイスを物理的に差し込むことができるポートに相当する場合もあれば、内部ポートに
 * 相当する場合もある。
 *
 * このハブドライバはUSBデバイスドライバの一例ではあるが、やや特殊なドライバである。
 * USBをサポートする場合はこのドライバの搭載が義務付けられているからである。
 * これはハブドライバがないとどんなUSBデバイスにもアクセスできないからである。
 * また、このハブドライバは usb_attach_device() など、他のUSBデバイスドライバには
 * 有用でないコアドライバのインタフェースを使用している。
 *
 * このUSBハブドライバの最初のエントリポイントは hub_bind_device() である。
 * USBコアは新しく接続されたUSBデバイスを構成した際にそれがハブだと思われる
 * 場合にこの関数を呼び出す。hub_bind_device() は、デバイスがハブであるか否を
 * チェックし、ハブであればドライバの一回限りの初期化、ハブディスクリプタの読み込み、
 * ポートへの電源投入、ハブのステータス変化エンドポイントへの非同期USBインターラプト
 * 転送要求の発行などのハブ固有の設定処理を行う役割を担っている。
 *
 * このハブドライバが行うこれ以外のすべての処理はステータス変化要求の完了に対する
 * 応答として非同期的に行われる。すべてのUSBハブは「ステータス変化エンドポイント」
 * と呼ばれるINインターラプトエンドポイントを1つだけ持っている。ハブのステータス、
 * または、ハブのポートに変更があった時、たとえば、USBデバイスがポートに着脱された
 * 時などに、ハブはこのエンドポイントに応答する。
 *
 * ハードウェアレベルでは、ハブにステータス変化エンドポイントに送信する
 * データがある場合、USBホストコントローラから割り込みが入る。これにより、
 * 最終的にはステータス変化転送が完了し、hub_status_changed() が呼び出される
 * ことになる。このようにステータス変化の検出は割り込み駆動であり、ソフトウェア
 * レベルのポーリングでは実装されていない。（ハードウェアレベルでは依然として、
 * USBはポーリングバスであるが、ホストコントローラハードウェアがそれを処理して
 * くれる）。ハブ上の1つ以上のポートでステータス変化を検出すると、ハブドライバは
 * ハブに1つ以上のコントロールメッセージを送信するので、影響を受けたポートがどう
 * 変化したのかを正確に判断する必要がある。ただし、割り込みハンドラで多くの同期
 * 処理を行うのを避けるため、この作業は別のスレッドに渡すことで先送りしている。
 */

#include <stdlib.h>
#include <thread.h>
#include <usb_core_driver.h>
#include <usb_hub_defs.h>
#include <usb_hub_driver.h>
#include <usb_std_defs.h>
#include <core.h>
#include <clock.h>

/** @ingroup usbhub
 * @def HUB_MAX_PORTS
 * このドライバがサポートするハブあたりの最大ポート数.
 * （USB 2.0は理論的にはハブあたり255個までのポートを許容している） */
#define HUB_MAX_PORTS 16

/** @ingroup usbhub
 * @struct usb_port
 * @brief USBポート構造体. USBポートのこのドライバでの表現 */
struct usb_port
{
    struct usb_hub *hub;        /**< 接続しているUSBハブへのポインタ */
    uint8_t number;             /**< ポート番号（1始まり） */
    struct usb_device *child;   /**< 接続されているUSBデバイスへのポインタ、未接続であればNULL */
    struct usb_port_status status;  /**< このポートのステータス  */
};

/** @ingroup usbhub
 * @struct usb_hub
 * @brief USBハブ構造体. USBハブのこのドライバでの表現  */
struct usb_hub
{
    bool inuse;                             /**< 使用されていれば TRUE */
    struct usb_device *device;              /**< このハブのUSBデバイス構造体へのポインタ  */
    struct usb_hub_descriptor descriptor;   /**< ハブディスクリプタ  */
    uint8_t descriptor_varData[64];         /**< ハブディスクリプタの末尾にある可変長データのための予約スペース. つまり、'descrptor' はこのフィールドにオーバーフローする。  */
    struct usb_port ports[HUB_MAX_PORTS];   /**< ポート配列. descriptor.bNbrPorts個だけ使用 */
};

/** @ingroup usbhub
 * @def MAX_NUSBHUBS
 * USBに同時に接続できるUSBハブの最大数
 */
#define MAX_NUSBHUBS 16

/** @ingroup usbhub
 * @var hub_status_change_data
 * ハブステータス変化データバッファの配列  */
static uint8_t hub_status_change_data[MAX_NUSBHUBS][8];

/** @ingroup usbhub
 * @var hub_status_change_requests
 * ハブステータス変化転送要求の配列  */
static struct usb_xfer_request hub_status_change_requests[MAX_NUSBHUBS];

/** @ingroup usbhub
 * @var hub_structs
 * ハブ構造体（ハブ固有データ）の配列  */
static struct usb_hub hub_structs[MAX_NUSBHUBS];

/** @ingroup usbhub
 * @var hub_status_change_pending
 * ステータス変化が保留されているハブのビットマスク. 注: hub_status_changed() の
 * 割り込みハンドラで変更される可能性がある  */
static uint32_t hub_status_change_pending;

/** @ingroup usbhub
 * @var hub_status_change_sema
 * ステータス変化が発生した時にハブスレッドに通知するためのセマフォ  */
static semaphore hub_status_change_sema;

/** @ingroup usbhub
 * @var hub_thread_tid
 * ハブスレッドのスレッドID (hub_thread())  */
static tid_typ hub_thread_tid = BADTID;

/** @ingroup usbhub
 * ハブ構造体と関連するステータス変化リクエストを割り当てる
 *
 * @return 割り当てたハブ番号、空きが無かった場合は SYSERR
 */
static int hub_alloc(void)
{
    uint i;
    /* 最後に割り当てた番号を保持している（+1して使用する） */
    static uint nexthub;

    /* ここでは割り込みを無効にしない。この関数はUSBコアによりシリアル化されている
     * bind_device コールバックからしか呼び出されないからである。  */

    for (i = 0; i < MAX_NUSBHUBS; i++)
    {
        nexthub = (nexthub + 1) % MAX_NUSBHUBS;
        if (!hub_structs[nexthub].inuse)
        {
            bzero(&hub_structs[nexthub], sizeof(struct usb_hub));
            hub_structs[nexthub].inuse = TRUE;
            return nexthub;
        }
    }
    return SYSERR;
}

/** @ingroup usbhub
 * ハブ構造体を解放する. ハブ構造体を空きだとマークする
 *
 * @param hubid 解放するハブの番号
 */
static void hub_free(int hubid)
{
    hub_structs[hubid].inuse = FALSE;
}

/** @ingroup usbhub
 * @def HUB_THREAD_STACK_SIZE
 * USBハブスレッドのスタックサイズ.  これはあまり大きくする必要はないが、
 * ハブスレッドはUSBデバイスドライバのコールバック関数 bind_device と
 * unbind_device を呼び出す場合がある  */
#define HUB_THREAD_STACK_SIZE 8192

/** @ingroup usbhub
 * @def HUB_THREAD_PRIORITY
 * USBハブスレッドの優先度.  USB接続の変化に素早く応答できるようにこれは
 * かなり高くするべきである。*/
#define HUB_THREAD_PRIORITY   60

/** @ingroup usbhub
 * @def HUB_THREAD_NAME
 * USBハブスレッドの名前 */
#define HUB_THREAD_NAME "USB hub thread"

/** @ingroup usbhub
 * ハブディスクリプタを読み込んでhub->descriptorに保存する.
 * 注: ハブディスクリプタはクラス固有のディスクリプであり、
 * 一般的なデバイスディスクリプタと同じではない
 *
 * @param hub ハブ構造体へのポインタ
 * @return usb_get_descriptor() が返す任意のコード
 */
static usb_status_t
hub_read_descriptor(struct usb_hub *hub)
{
    usb_status_t status;
    usb_dev_debug(hub->device, "Reading hub descriptor.\n");
    status = usb_get_descriptor(hub->device,
                                USB_HUB_REQUEST_GET_DESCRIPTOR,
                                USB_BMREQUESTTYPE_DIR_IN |
                                    USB_BMREQUESTTYPE_TYPE_CLASS |
                                    USB_BMREQUESTTYPE_RECIPIENT_DEVICE,
                                (USB_DESCRIPTOR_TYPE_HUB << 8), 0,
                                &hub->descriptor, sizeof(hub->descriptor) +
                                                  sizeof(hub->descriptor_varData));
    if (status != USB_STATUS_SUCCESS)
    {
        usb_dev_error(hub->device, "Failed to read hub descriptor: %s\n",
                      usb_status_string(status));
    }
    return status;
}

/** @ingroup usbhub
 * USBポートのステータスを取得して port->status に保存する.
 *
 * @param port USBポート構造体へのポインタ
 * @return usb_control_msg() が返す任意のコード
 */
static usb_status_t
port_get_status(struct usb_port *port)
{
    usb_status_t status;

    usb_dev_debug(port->hub->device,
                  "Retrieving status of port %u\n", port->number);
    status = usb_control_msg(port->hub->device, NULL,
                             USB_HUB_REQUEST_GET_STATUS,
                             USB_BMREQUESTTYPE_DIR_IN |
                                 USB_BMREQUESTTYPE_TYPE_CLASS |
                                 USB_BMREQUESTTYPE_RECIPIENT_OTHER,
                             0, port->number,
                             &port->status, sizeof(port->status));
    usb_dev_debug(port->hub->device, "Got port status\n");
    if (status != USB_STATUS_SUCCESS)
    {
        usb_dev_error(port->hub->device,
                      "Failed to get status for port %u: %s\n",
                      port->number, usb_status_string(status));
    }
    return status;
}

/** @ingroup usbhub
 *
 * USBポートの機能を有効/無効にする.  これはUSB 2.0仕様のセクション11.24で
 * 定義されている ClearPortFeature と SetPortFeature に対応する。
 *
 * @param port
 *      USBポート構造体へのポインタ.
 * @param feature
 *      有効/無効にする機能.
 * @param enable
 *      機能を有効にする場合は TRUE; 無効にする場合は FALSE.
 *
 * @return
 *      usb_control_msg() が返す任意のコード.
 */
static usb_status_t
port_change_feature(struct usb_port *port, enum usb_port_feature feature,
                    bool enable)
{
    uint8_t bRequest = (enable) ? USB_HUB_REQUEST_SET_FEATURE :
                                  USB_HUB_REQUEST_CLEAR_FEATURE;
    return usb_control_msg(port->hub->device, NULL,
                           bRequest,
                           USB_BMREQUESTTYPE_DIR_OUT |
                               USB_BMREQUESTTYPE_TYPE_CLASS |
                               USB_BMREQUESTTYPE_RECIPIENT_OTHER,
                           feature, port->number, NULL, 0);
}

/** @ingroup usbhub
 * 指定のUSBポート機能を有効にする.
 *
 * @param port ポート構造体へのポインタ
 * @param feature 有効にするポート機能
 * @return usb_control_msg() が返す任意のコード
*/
static usb_status_t
port_set_feature(struct usb_port *port, enum usb_port_feature feature)
{
    return port_change_feature(port, feature, TRUE);
}

/** @ingroup usbhub
 * 指定のUSBポート機能を無効にする.
 *
 * @param port ポート構造体へのポインタ
 * @param feature 無効にするポート機能
 * @return usb_control_msg() が返す任意のコード
*/
static usb_status_t
port_clear_feature(struct usb_port *port, enum usb_port_feature feature)
{
    return port_change_feature(port, feature, FALSE);
}

/** @ingroup usbhub
 * @def USB_PORT_RESET_TIMEOUT
 * ポートがリセットされるのを待つ最大ミリ秒数（800 はLinuxが仕様している値と同じ） */
#define USB_PORT_RESET_TIMEOUT 800

/** @ingroup usbhub
 * @def USB_PORT_RESET_DELAY
 * ポートのリセット完了を待つ際にステータスチェックを行う間隔のミリ秒数
 * （Linuxはいくつかの値を使用しているが、デフォルトは10）
 */
#define USB_PORT_RESET_DELAY    10

/** @ingroup usbhub
 * USBポートをリセットする.
 *
 * @param port
 *      USBポート構造体へのポインタ.
 *
 * @return
 *      usb_control_msg() が返す任意の値.  リセットの完了を確認する
 *      のに非常に時間がかかる場合は ::USB_STATUS_TIMEOUT
 */
static usb_status_t
port_reset(struct usb_port *port)
{
    usb_status_t status;
    uint i;

    usb_dev_debug(port->hub->device, "Resetting port %u\n", port->number);

    /* ハードウェアにリセットするよう伝える */
    status = port_set_feature(port, USB_PORT_RESET);
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }

    /* ハードウェアがポートステータスの USB_PORT_RESET フラグをクリアする
     * まで待機する。規定の時間が立ってもフラグがクリアされなかった場合は
     * USB_STATUS_TIMEOUT を返す。 */
    for (i = 0; i < USB_PORT_RESET_TIMEOUT && port->status.reset;
         i += USB_PORT_RESET_DELAY)
    {
        sleep(USB_PORT_RESET_DELAY);
        status = port_get_status(port);
        /* エラーが発生したらエラーで復帰する */
        if (status != USB_STATUS_SUCCESS)
        {
            return status;
        }
    }
    /* タイムアウトで復帰する */
    if (i >= USB_PORT_RESET_TIMEOUT)
    {
        return USB_STATUS_TIMEOUT;
    }

    /* USB 2.0 仕様のセクション 9.2.6.2より:
     *
     *   「ポートがリセットまたは再開されたら、USB システムソフトウェアは
     *   ポートに接続されたデバイスがデータ転送に応答する前に 10 ms の
     *   『回復』間隔を設けることが期待されている。デバイスは回復間隔中の
     *   データ転送を無視することができる。」
     *
     * どうやら、これよりさらに遅いデバイスもあるようなのでこれより大きな
     * 回復時間を設けている。
     */
    sleep(30);

    return USB_STATUS_SUCCESS;
}

/** @ingroup usbhub
 * ポートへのデバイス取り付け関数.
 * 新しいUSBデバイスがUSBポートに接続された際に呼び出される。
 * この関数はポートをリセットし、USBコアドライバを呼び出して、
 * ポートに接続された新しいデバイスにアドレスを設定し、構成を
 * 行う必要がある。
 *
 * @param port ポート構造体へのポインタ
 */
static void
port_attach_device(struct usb_port *port)
{
    usb_status_t status;
    struct usb_device *new_device;

    status = port_reset(port);
    if (status != USB_STATUS_SUCCESS)
    {
        usb_dev_error(port->hub->device, "Failed to reset port %u: %s\n",
                      port->number, usb_status_string(status));
        return;
    }

    /* 接続されたデバイスのためのデバイス構造体を割り当てる */
    new_device = usb_alloc_device(port->hub->device);
    if (new_device == NULL)
    {
        usb_error("Too many USB devices attached\n");
        status = USB_STATUS_OUT_OF_MEMORY;
        port_clear_feature(port, USB_PORT_ENABLE);
        return;
    }

    /* ポートステータスを取得する */
    status = port_get_status(port);
    if (status != USB_STATUS_SUCCESS)
    {
        usb_free_device(new_device);
        port_clear_feature(port, USB_PORT_ENABLE);
        return;
    }

    /* 接続されたデバイスのスピードを記録する。ハードウェアはそれが何かを
     * 知っているので、ポートのステータス構造体からそれを取得するだけである。 */
    if (port->status.high_speed_attached)
    {
        new_device->speed = USB_SPEED_HIGH;
    }
    else if (port->status.low_speed_attached)
    {
        new_device->speed = USB_SPEED_LOW;
    }
    else
    {
        new_device->speed = USB_SPEED_FULL;
    }

    usb_dev_info(port->hub->device,
                 "New %s-speed device connected to port %u\n",
                 usb_speed_to_string(new_device->speed), port->number);
    new_device->port_number = port->number;

    /* 接続されたデバイスのenumerationを行う */
    status = usb_attach_device(new_device);
    if (status != USB_STATUS_SUCCESS)
    {
        usb_dev_error(port->hub->device,
                      "Failed to attach new device to port %u: %s\n",
                      port->number, usb_status_string(status));
        usb_free_device(new_device);
        port_clear_feature(port, USB_PORT_ENABLE);
        return;
    }
    /* デバイスをポートに登録する */
    usb_lock_bus();
    port->child = new_device;
    usb_unlock_bus();
}

/** @ingroup usbhub
 * ポートからのデバイス取り外し関数.
 * （もしあれば）USBポートに接続されていたUSBデバイスからドライバを
 * アンバインドしてデバイス構造体を解放する。
 *
 * @param port ポート構造体へのポインタ
 */
static void
port_detach_device(struct usb_port *port)
{
    if (port->child != NULL)
    {
        usb_lock_bus();
        usb_dev_debug(port->hub->device, "Port %u: device detached.\n",
                      port->number);
        usb_info("Detaching %s\n", usb_device_description(port->child));
        usb_free_device(port->child);
        port->child = NULL;
        usb_unlock_bus();
    }
}

/** @ingroup usbhub
 * ポートステータス変化完了時のコールバック関数.
 * USBポートのステータス変化に応答する。
 *
 * @param port ポート構造体へのポインタ
 */
static void
port_status_changed(struct usb_port *port)
{
    usb_status_t status;

    /* USBコントロールメッセージを送信してポートステータスを取得する。
     * ステータス変化はport->status に格納されており、返り値の `status`
     * ではない。返り値はポートステータスの取得に対するステータスである）
     */
    status = port_get_status(port);
    if (status != USB_STATUS_SUCCESS)
    {
        return;
    }

    usb_dev_debug(port->hub->device,
                  "Port %u: {wPortStatus=0x%04x, wPortChange=0x%04x}\n",
                  port->number,
                  port->status.wPortStatus,
                  port->status.wPortChange);

    /* ステータス変化を処理する */
    /* 1. デバイスが接続、または切断された */
    if (port->status.connected_changed)
    {
        usb_dev_debug(port->hub->device, "Port %u: device now %s\n",
                      port->number,
                      (port->status.connected ? "connected" : "disconnected"));

        /* 接続変化フラグをクリア/確認する */
        port_clear_feature(port, USB_C_PORT_CONNECTION);

        /* （もしあれば）古いデバイスを取り外す */
        port_detach_device(port);

        if (port->status.connected)
        {
            /* 新しいデバイスを接続する */
            port_attach_device(port);
        }
    }

    /* 2. 接続状態以外の変化はフラグをクリアするだけ */

    if (port->status.enabled_changed)
    {
        port_clear_feature(port, USB_C_PORT_ENABLE);
    }

    if (port->status.reset_changed)
    {
        port_clear_feature(port, USB_C_PORT_RESET);
    }

    if (port->status.suspended_changed)
    {
        port_clear_feature(port, USB_C_PORT_SUSPEND);
    }

    if (port->status.overcurrent_changed)
    {
        port_clear_feature(port, USB_C_PORT_OVER_CURRENT);
    }
}

/**
 * @ingroup usbhub
 *
 * デバイスツリー内の各USBデバイスに対して指定の関数を実行する.
 * これはUSB上のすべてのデバイスに何らかの処理を繰り返し行うために
 * 使用することができる。この関数を呼び出すコードはこの関数の実行中に
 * デバイスがUSBから切り離されないようにする責任がある（たとえば、
 * 割り込みを無効にして実行する、usb_lock_bus() を呼び出すなど）。
 *
 * @param dev
 *      繰り返し処理を始めるUSBデバイスツリーのルート
 * @param callback
 *      各デバイスに対して実行するコールバック関数
 */
void usb_hub_for_device_in_tree(struct usb_device *dev,
                                usb_status_t (*callback)(struct usb_device *))
{
    if (dev != NULL)
    {
        (*callback)(dev);
        if (is_hub(dev))
        {
            int hub_id = (int)dev->driver_private;
            struct usb_hub *hub = &hub_structs[hub_id];
            uint i;

            for (i = 0; i < hub->descriptor.bNbrPorts; i++)
            {
                usb_hub_for_device_in_tree(hub->ports[i].child, callback);
            }
        }
    }
}

/** @ingroup usbhub
 * ハブステータス変化処理スレッド.
 *
 * USB上にハブがいくつあってもインスタンスは１つだけ存在する. ハブスレッドは
 * 完了したハブのステータス変化リクエストを繰り返し取得し、ステータス変化が
 * あったポートに関するより詳細な情報を取得してポートのステータス変化を処理し、
 * 応答する責任がある。
 *
 * 各ステータス変化要求は処理された後、再発行される。
 *
 * このスレッドは意図的に割り込みを有効にして実行することでプリエンプション
 * を可能にしている。
 *
 * @return
 *      このスレッドは復帰しない
 */
static thread
hub_thread(void)
{
    for (;;)
    {
        /* 1つ以上のハブステータス変化メッセージが到着するのを待機して、
         * 処理する  */
        wait(hub_status_change_sema);
        while (hub_status_change_pending != 0)
        {
            struct usb_xfer_request *req;
            struct usb_hub *hub;
            int hub_id;
            irqmask im;

            hub_id = 31 - __builtin_clz(hub_status_change_pending);
            req = &hub_status_change_requests[hub_id];
            hub = &hub_structs[hub_id];

            /* このハブ用のステータス変化保留ビットをクリアする。
             * ただし、hub_status_changed() が *別の* ハブの同じビット
             * マスクを変更する競合を避けるために一時的に割り込みを
             * 無効にする。 */
            im = disable();
            hub_status_change_pending &= ~(1 << hub_id);
            restore(im);

            if (req->status == USB_STATUS_SUCCESS)
            {
                uint32_t portmask;
                uint i;

                usb_dev_debug(req->dev, "Processing hub status change\n");

                // XXX TODO delay moves along execution for some reason...
                udelay(25);
                /* メッセージフォーマットは、どのポートのステータスに変化が
                 * あったかを示すビットマップである。ハブデバイス自体の
                 * ステータス変化を示すビット0 は無視する。
                 * （この実装では常にreq->actual_size=1で、req->recvbuf[0] = 0x2 */
                portmask = 0;
                for (i = 0; i < req->actual_size; i++)
                {
                    portmask |= (uint32_t)((uint8_t*)req->recvbuf)[i] << (i * 8);
                }

                /* ステータス変化が検知されたポートを処理する(port 0を除く)
                   （この実装ではbNbrPortsは常に1） */
                for (i = 0; i < hub->descriptor.bNbrPorts; i++)
                {
                    if (portmask & (2 << i))
                    {
                        port_status_changed(&hub->ports[i]);
                    }
                }
            }
            else
            {
                usb_dev_error(req->dev, "Status change request failed: %s\n",
                              usb_status_string(req->status));
            }

            /* ステータス変化リクエストを再発行する  */
            usb_dev_debug(req->dev, "Re-submitting status change request\n");
            usb_submit_xfer_request(req);
        }
    }
    return SYSERR;
}

/** @ingroup usbhub
 * ハブステータス変化処理コールバック関数.
 * ハブがステータス変化パイプ（INインターラプトエンドポイント）でデータを
 * 受信したときに呼び出されるコールバック関数である.
 *
 * これは割り込みハンドラから呼ばれるのでこの関数の中でステータス変化を
 * 処理したくはない。代わりに hub_thread() にステータ変化が発生したことを
 * 通知して処理を行わせる。
 *
 * @param req
 *      完了したハブステータス変化エンドポイントからのUSB転送リクエスト
 */
static void
hub_status_changed(struct usb_xfer_request *req)
{
    int hub_id;

    /* hub_status_change_requests は struct usb_xfer_request の配列で、
     * reqはその配列要素なので、reqから配列の先頭を引くとそのインデックスとなる */
    hub_id = req - hub_status_change_requests;
    /* 保留ビットを建てる */
    hub_status_change_pending |= 1 << hub_id;
    /* 変化処理スレッドにシグナルを送って処理をさせる */
    signal(hub_status_change_sema);
}

/** @ingroup usbhub
 * USBハブドライバの一回限りの初期化.
 *
 * @return 初期化に成功（初期化済みの場合を含む）した場合は USB_STATUS_SUCCESS;
 *         それ以外は ::usb_status_t 型のエラーコード
 */
static usb_status_t
hub_onetime_init(void)
{
    uint i;

    if (BADTID != hub_thread_tid)
    {
        /* 初期化済み */
        return USB_STATUS_SUCCESS;
    }

    /* ハブスレッドに通知するためのセマフォを作成する */
    hub_status_change_sema = semcreate(0);
    if (SYSERR == hub_status_change_sema)
    {
        return USB_STATUS_OUT_OF_MEMORY;
    }

    /* static宣言されているステータス変化要求配列を初期化する */
    hub_status_change_pending = 0;
    for (i = 0; i < MAX_NUSBHUBS; i++)
    {
        usb_init_xfer_request(&hub_status_change_requests[i]);
        hub_status_change_requests[i].recvbuf = hub_status_change_data[i];          /* uint8_t[8] */
        hub_status_change_requests[i].size = sizeof(hub_status_change_data[i]);     /* 8 */
        hub_status_change_requests[i].completion_cb_func = hub_status_changed;
        hub_structs[i].inuse = FALSE;
    }

    /* ハブスレッドを作成して起動する */
    hub_thread_tid = create(hub_thread, HUB_THREAD_STACK_SIZE, HUB_THREAD_PRIORITY,
                            HUB_THREAD_NAME, 0);
    if (SYSERR == ready(hub_thread_tid, RESCHED_NO, CORE_ZERO))
    {
        kill(hub_thread_tid);
        hub_thread_tid = BADTID;
        semfree(hub_status_change_sema);
        return USB_STATUS_OUT_OF_MEMORY;
    }
    return USB_STATUS_SUCCESS;
}

/** @ingroup usbhub
 * USBハブポート初期化関数.
 * USBハブのポート構造体を初期化する; また、ハブがこのドライバがサポートする
 * ポート数を超えるポートを持っていないかチェックする。
 *
 * @param hub ハブ構造体へのポインタ
 * @return 初期化に成功したら USB_STATUS_SUCCESS; ポート数がこのドライバの
 *         最大ポート数を超える場合は USB_STATUS_DEVICE_UNSUPPORTED
 */
static usb_status_t
hub_init_ports(struct usb_hub *hub)
{
    uint i;

    if (hub->descriptor.bNbrPorts > HUB_MAX_PORTS)
    {
        usb_dev_error(hub->device,
                      "Too many ports on hub (%u > HUB_MAX_PORTS=%u)\n",
                      hub->descriptor.bNbrPorts, HUB_MAX_PORTS);
        return USB_STATUS_DEVICE_UNSUPPORTED;
    }

    for (i = 0; i < hub->descriptor.bNbrPorts; i++)
    {
        hub->ports[i].hub = hub;
        hub->ports[i].number = i + 1;
    }

    return USB_STATUS_SUCCESS;
}

/** @ingroup usbhub
 * USBハブのすべてのポートの電源を入れる.
 *
 * @param hub ハブ構造体へのポインタ
 * @return すべての電源のオンに成功したら USB_STATUS_SUCCESS;
 *         それ以外は port_set_feature() が返す任意のコード
 */
static usb_status_t
hub_power_on_ports(struct usb_hub *hub)
{
    uint i;
    usb_status_t status;

    usb_dev_debug(hub->device, "Powering on %u USB ports\n",
                  hub->descriptor.bNbrPorts);

    for (i = 0; i < hub->descriptor.bNbrPorts; i++)
    {
        status = port_set_feature(&hub->ports[i], USB_PORT_POWER);
        if (status != USB_STATUS_SUCCESS)
        {
            usb_dev_error(hub->device,
                          "Failed to power on port %u: %s\n", i,
                          usb_status_string(status));
            return status;
        }
    }

    /* USB 2.0仕様のセクション11.11によると、ハブディクリプタの
     * bPwrOn2PwrGoodは「あるポートでの電源投入シーケンスの開始から
     * そのポートで電源が安定するまでの時間（2ms間隔）」である。
     * ここでこの必要な遅延時間を挿入する。  */
    sleep(2 * hub->descriptor.bPwrOn2PwrGood);

    return USB_STATUS_SUCCESS;
}

/** @ingroup usbhub
 * 新しいUSBデバイスをハブドライバにバインドする.  これはハブ
 * ドライバ用の @ref usb_device_driver::bind_device "bind_device" の
 * 実装であるのでその文書化されている動作に準拠している。ただし、
 * このハブドライバは複数の同時接続ハブを完全にサポートしている
 * （正確には ::MAX_NUSBHUBS 個まで）ことが実装上、重要である。
 *
 * @param dev USBデバイスへのポインタ
 * @return バインドに成功したら USB_STATUS_SUCCESS; それ以外は
 *         ::usb_status_t 型のエラーコード
 */
static usb_status_t
hub_bind_device(struct usb_device *dev)
{
    usb_status_t status;
    struct usb_hub *hub;
    int hub_id;

    /* 新しいデバイスがハブであるかチェックする  */
    if (dev->descriptor.bDeviceClass != USB_CLASS_CODE_HUB ||
        dev->config_descriptor->bNumInterfaces != 1 ||
        dev->interfaces[0]->bNumEndpoints != 1 ||
        (dev->endpoints[0][0]->bmAttributes & 0x3) !=
                USB_TRANSFER_TYPE_INTERRUPT)
    {
        return USB_STATUS_DEVICE_UNSUPPORTED;
    }

    /* ハブドライバの一回限りの初期化を行う */
    status = hub_onetime_init();
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }

    /* ハブIDを割り当てる  */
    hub_id = hub_alloc();
    if (SYSERR == hub_id)
    {
        usb_error("Too many hubs attached.\n");
        return USB_STATUS_DEVICE_UNSUPPORTED;
    }

    /* ハブ構造体を割り当てる */
    hub = &hub_structs[hub_id];
    hub->device = dev;

    /* ハブディクリプタを読み込む  */
    status = hub_read_descriptor(hub);
    if (status != USB_STATUS_SUCCESS)
    {
        hub_free(hub_id);
        return status;
    }

    usb_dev_debug(dev, "Attaching %sUSB hub with %u ports\n",
                  (hub->descriptor.wHubCharacteristics &
                   USB_HUB_CHARACTERISTIC_IS_COMPOUND_DEVICE) ?
                            "compound device " : "",
                  hub->descriptor.bNbrPorts);

    /* USBポートを初期化する  */
    status = hub_init_ports(hub);
    if (status != USB_STATUS_SUCCESS)
    {
        hub_free(hub_id);
        return status;
    }

    /* このハブに接続されているポートの電源を入れる */
    status = hub_power_on_ports(hub);
    if (status != USB_STATUS_SUCCESS)
    {
        hub_free(hub_id);
        return status;
    }

    /* ステータス変化リクエストを発行する  */
    hub_status_change_requests[hub_id].dev = dev;
    hub_status_change_requests[hub_id].endpoint_desc = dev->endpoints[0][0];
    dev->driver_private = (void*)hub_id;
    status = usb_submit_xfer_request(&hub_status_change_requests[hub_id]);
    if (status != USB_STATUS_SUCCESS)
    {
        hub_free(hub_id);
        return status;
    }
    return USB_STATUS_SUCCESS;
}

/** @ingroup usbhub
 * デタッチされたハブからハブドライバをアンバインドする.  これはハブドライバ用の
 * @ref usb_device_driver::unbind_device "unbind_device" の実装であるので
 * その文書化されている動作に準じている。ただし、ハブドライバだけが子デバイスを
 * 再帰的にデタッチする必要があることが重要である。
 */
static void
hub_unbind_device(struct usb_device *hub_device)
{
    int hub_id = (int)hub_device->driver_private;
    struct usb_hub *hub = &hub_structs[hub_id];
    irqmask im;
    uint i;

    /* このハブに接続されているすべてのデバイス（「子」デバイス）を取り外す  */
    for (i = 0; i < hub->descriptor.bNbrPorts; i++)
    {
        if (hub->ports[i].child != NULL)
        {
            usb_free_device(hub->ports[i].child);
        }
    }

    /* ハブスレッドによる処理を待っているステータス変化リクエストのマスク変数から
     * 取り外されたハブ用のステータス変化リクエストを削除する.  これを行うには
     * hub_status_change_pending の対応するビットをクリアする必要がある。
     * 「別の」ハブの同じビットマスクのビットを設定しようとする
     * hub_status_changed() と競合しないように割り込みを一時的に無効にする
     * 必要がある。 */
    im = disable();
    hub_status_change_pending &= ~(1 << hub_id);
    restore(im);

    /* 取り外されたハブ用のusb_hub構造体を解放する */
    hub_free(hub_id);
}

/**
 * @ingroup usbhub
 * @var usb_hub_driver
 * USBハブデバイスドライバの宣言.
 */
const struct usb_device_driver usb_hub_driver = {
    .name          = "USB Hub Driver",
    .bind_device   = hub_bind_device,
    .unbind_device = hub_unbind_device,
};
