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
 * するための接続ポイント（ポート）を提供するために使用される。たとえ、「外部」
 * ハブが接続されていない場合でも、USBは少なくとも1つ論理ハブ（ルートハブ）を
 * 持っており、通常は、さらに「内部」ハブが存在することに注意されたい。すわわち、
 * USBはデバイスのツリーであり、ルートノードと非リーフのすべてのノードがハブである。
 * USBハブのポートは、デバイスを物理的に差し込むことができるポートに相当する
 * 場合もあれば、内部ポートに相当する場合もある。
 *
 * このハブドライバはUSBデバイスドライバの一例であるがやや特殊なドライバである。
 * USBをサポートする場合はこのドライバの搭載が義務付けられているからである。
 * これはハブドライバがないとあらゆるUSBデバイスにアクセスできないからである。
 * また、このハブドライバは usb_attach_device() など、他のUSBデバイスドライバには
 * 有用でないコアドライバのインタフェースを使用している。
 *
 * このUSBハブドライバの最初のエントリポイントは hub_bind_device() である。
 * USBコアはハブだと思われる新しく接続されたUSBデバイスを構成した際にこの関数を
 * 呼び出す。hub_bind_device() は、デバイスがハブであるかどうかをチェックし、
 * そうである場合、ドライバの一回限りの初期化、ハブディスクリプタの読み込み、
 * ポートへの電源投入、ハブのステータス変化エンドポイントへの非同期USBインターラプト
 * 転送リクエストなどのハブ固有のセットアップを行う役割を担っている。
 *
 * このハブドライバが行う他のすべての処理はステータス変化リクエストの完了に対する
 * 応答として非同期的に行われる。すべてのUSBハブは「ステータス変化エンド
 * ポイント」と呼ばれるINインターラプトエンドポイントを1つだけ持っている。ハブの
 * ステータスまたはハブのポートに変更があった時、たとえば、USBデバイスがポートに
 * 着脱された時などは常に、ハブはこのエンドポイントに応答する。
 *
 * ハードウェアレベルでは、ハブにステータス変化エンドポイントに送信する
 * データがある場合、USBホストコントローラから割り込みが入る。これにより、
 * 最終的にはステータス変化転送が完了し、hub_status_changed() が呼び出される
 * ことになる。このようにステータス変化の検出は割り込み駆動であり、ソフトウェア
 * レベルのポーリングでは実装されていない。（ハードウェアレベルでは依然として、
 * USBはポーリングバスであるが、ホストコントローラハードウェアがそれを処理して
 * くれる）。ハブ上の1つ以上のポートでステータス変化を検出すると、ハブドライバは
 * ハブに1つ以上のコントロールメッセージを送信し、影響を受けたポートがどのように
 * 変化したのかを正確に判断する必要がある。しかし、割り込みハンドラで多くの同期
 * 処理を行うのを避けるため、この作業は別のスレッドに渡すことで先送りしている。
 */

#include <stdlib.h>
#include <thread.h>
#include <usb_core_driver.h>
#include <usb_hub_defs.h>
#include <usb_hub_driver.h>
#include <usb_std_defs.h>

/** このドライバがサポートするハブあたりの最大ポート数.
 * （USB 2.0は理論的にはハブあたり255個までのポートを許容している） */
#define HUB_MAX_PORTS 16

/** USBポートのこのドライバでの表現  */
struct usb_port
{
    /** このポートが接続しているUSBハブへのポインタ */
    struct usb_hub *hub;

    /** このポートの番号（1始まり） */
    uint8_t number;

    /** このポートに接続されているUSｂデバイスへのポインタ、
     * 接続されていない場合は NULL */
    struct usb_device *child;

    /** このポートのステータス  */
    struct usb_port_status status;
};

/** USBハブのこのドライバでの表現  */
struct usb_hub
{
    /** このハブ構造体が使用されている場合は TRUE */
    bool inuse;

    /** このハブのUSBデバイスへのポインタ  */
    struct usb_device *device;

    /** このハブのディスクリプタ  */
    struct usb_hub_descriptor descriptor;

    /** ハブディスクリプタの末尾にある可変長データのための予約スペース.
     * すなわち、'descrptor' はこのフィールドにオーバーフローする。  */
    uint8_t descriptor_varData[64];

    /** このハブのポート配列. 実際には最初の descriptor.bNbrPorts 個の
     * エントリだけが使用される */
    struct usb_port ports[HUB_MAX_PORTS];
};

/** USBに同時に接続できるUSBハブの最大数
 */
#define MAX_NUSBHUBS 16

/** ハブステータス変化データバッファ  */
static uint8_t                 hub_status_change_data[MAX_NUSBHUBS][8];

/** ハブステータス変化転送リクエスト  */
static struct usb_xfer_request hub_status_change_requests[MAX_NUSBHUBS];

/** ハブ固有のデータ構造体  */
static struct usb_hub          hub_structs[MAX_NUSBHUBS];

/** ステータス変化が保留されているハブのビットマスク. 注: hub_status_changed() の
 * 割り込みハンドラで変更される可能性がある  */
static uint32_t hub_status_change_pending;

/** ステータス変化が発生した時にハブスレッドに通知するためのセマフォ  */
static semaphore hub_status_change_sema;

/** フブスレッドのスレッドID (hub_thread())  */
static tid_typ hub_thread_tid = BADTID;

/** ハブ構造体と関連のステータス変化リクエストを割り当てる  */
static int hub_alloc(void)
{
    uint i;
    static uint nexthub;

    /* 個々では割り込みを無効にしない。この関数はUSBコアによりシリアル化されている
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

/** ハブ構造体を空きだとマークする */
static void hub_free(int hubid)
{
    hub_structs[hubid].inuse = FALSE;
}

/** USBハブスレッドのスタックサイズ.  これはあまり大きくする必要はないが、
 * ハブスレッドはUSBデバイスドライバのコールバック関数 bind_device と
 * unbind_device を呼び出すことができる  */
#define HUB_THREAD_STACK_SIZE 8192

/** USBハブスレッドの優先度.  USB接続の変化に素早く応答できるようにこれは
 * 非常に高くするべきである。  */
#define HUB_THREAD_PRIORITY   60

/** USBハブスレッドの名前  */
#define HUB_THREAD_NAME "USB hub thread"

/** ハブディスクリプタを読み込んでhub->descriptorに保存する.  注: ハブ
 * ディスクリプタはクラス固有のディスクリプであり、一般的なデバイス
 * ディスクリプタと同じではない  */
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

/** USBポートのステータスを取得して port->status に保存する.  */
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

/**
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
 *      usb_control_msg() を参照.
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

static usb_status_t
port_set_feature(struct usb_port *port, enum usb_port_feature feature)
{
    return port_change_feature(port, feature, TRUE);
}

static usb_status_t
port_clear_feature(struct usb_port *port, enum usb_port_feature feature)
{
    return port_change_feature(port, feature, FALSE);
}

/** ポートがリセットされるのを待つ最大ミリ秒数（800 はLinuxが仕様している値と同じ） */
#define USB_PORT_RESET_TIMEOUT 800

/** ポートのリセット完了を待つ際にステータスチェックを行う間隔のミリ秒数
 * （Linuxはいくつかの値を使用しているが、デフォルトは10）
 */
#define USB_PORT_RESET_DELAY    10

/**
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

    /* ハードウェアにリセットする湯伝える */
    status = port_set_feature(port, USB_PORT_RESET);
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }

    /* ハードウェアがポートステータスの USB_PORT_RESET フラグをクリアする
     * まで待機する。規定の時間が立ってもこれが生じなかった場合は
     * USB_STATUS_TIMEOUT を返す。 */
    for (i = 0; i < USB_PORT_RESET_TIMEOUT && port->status.reset;
         i += USB_PORT_RESET_DELAY)
    {
        sleep(USB_PORT_RESET_DELAY);
        status = port_get_status(port);
        if (status != USB_STATUS_SUCCESS)
        {
            return status;
        }
    }

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

/** この関数は新しいUSBデバイスがUSBポートに接続された際に呼び出される.
 * この関数はポートをリセットして、USBコアドライバを呼び出して、
 * ポートに接続された新しいデバイスにアドレスを設定し、構成する必要が
 * ある。  */
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

    new_device = usb_alloc_device(port->hub->device);
    if (new_device == NULL)
    {
        usb_error("Too many USB devices attached\n");
        status = USB_STATUS_OUT_OF_MEMORY;
        port_clear_feature(port, USB_PORT_ENABLE);
        return;
    }

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
    usb_lock_bus();
    port->child = new_device;
    usb_unlock_bus();
}

/** （もしあれば）USBポートに接続されていたUSBデバイスからドライバを
 * アンバインドしてデバイス構造体を開放する */
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

/** USBポートのステータス変化に応答する.  */
static void
port_status_changed(struct usb_port *port)
{
    usb_status_t status;

    /* USBコントロールメッセージを送信してポートステータスを取得する。
     * これはport->status に格納されており、返り値の `status` と
     * 同じではない。後者はポートステータスを取得したことのステータスで
     * あるからである）  */
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

    /* 様々なタイプのステータス変化を処理する */

    if (port->status.connected_changed)
    {
        /* 接続の変化: デバイスが接続、または切断された */

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
 * ツリー内の各USBデバイスに対して関数を呼び出す.  これはUSB上のすべての
 * デバイスを繰り返し処理するために使用することができる。呼び出し側の
 * コードはこの関数の実行中にデバイスがUSBから切り離されないようにする
 * 責任がある（たとえば、割り込みを無効にして実行する、または、
 * usb_lock_bus() を呼び出すなど）。
 *
 * @param dev
 *      繰り返し処理を始めるUSBデバイスツリーのルートデバイス
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

/**
 * ハブスレッドが実行するルーチンであり、USB上にハブがいくつあっても
 * インスタンスは１つだけ存在する. ハブスレッドは完了したハブのステータス
 * 変化リクエストを繰り返し取得し、ステータス変化があったポートに関する、
 * より詳細な情報を取得することによりそれを処理し、それに応答する責任が
 * ある。
 *
 * 各ステータス変化リクエストは処理された後。再発行される。
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

            /* このハブ用のステータス変化捕虫ビットをクリアする。
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

                /* メッセージフォーマットは、どのポートのステータスに変化が
                 * あったかを示すビットマップである。ハブデバイス自体の
                 * ステータス変化を示すビット0 は無視する。 */
                portmask = 0;
                for (i = 0; i < req->actual_size; i++)
                {
                    portmask |= (uint32_t)((uint8_t*)req->recvbuf)[i] << (i * 8);
                }

                /* ステータス変化が検知されたポートを処理する */
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

/**
 * ハブがステータス変化パイプ（INインターラプトエンドポイント）でデータを
 * 受信したときに呼び出されるコールバック関数である.
 *
 * これは割り込みハンドラから呼ばれるのでこの関数の中でステータス変化を
 * 処理したくはない。代わりに hub_thread() にステータ変化が発生したことを
 * 通知する。
 *
 * @param req
 *      完了したハブステータス変化エンドポイントからのUSB転送リクエスト
 */
static void
hub_status_changed(struct usb_xfer_request *req)
{
    int hub_id;

    hub_id = req - hub_status_change_requests;
    hub_status_change_pending |= 1 << hub_id;
    signal(hub_status_change_sema);
}

/**
 * USBハブドライバの一回限りの初期化.
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

    /* 利用可能なステータス変化リクエストとハブ構造体を初期化する */
    hub_status_change_pending = 0;
    for (i = 0; i < MAX_NUSBHUBS; i++)
    {
        usb_init_xfer_request(&hub_status_change_requests[i]);
        hub_status_change_requests[i].recvbuf = hub_status_change_data[i];
        hub_status_change_requests[i].size = sizeof(hub_status_change_data[i]);
        hub_status_change_requests[i].completion_cb_func = hub_status_changed;
        hub_structs[i].inuse = FALSE;
    }

    /* ハブスレッドを作成する */
    hub_thread_tid = create(hub_thread, HUB_THREAD_STACK_SIZE, HUB_THREAD_PRIORITY,
                            HUB_THREAD_NAME, 0);
    if (SYSERR == ready(hub_thread_tid, RESCHED_NO))
    {
        kill(hub_thread_tid);
        hub_thread_tid = BADTID;
        semfree(hub_status_change_sema);
        return USB_STATUS_OUT_OF_MEMORY;
    }
    return USB_STATUS_SUCCESS;
}

/**
 * USBハブのポート構造体を初期化する; また、ハブがこのドライバがサポートする
 * より多くのポートを持ていないかチェックする。
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

/**
 * PUSBハブのすべてのポートの電源を入れる.
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

/**
 * 新しいUSBデバイスのハブドライバへのバインドを試みる.  これはハブ
 * ドライバ用の @ref usb_device_driver::bind_device "bind_device" の
 * 実装であるのでその文書化されている動作に準拠している。ただし、
 * 重要な実装の詳細として、ハブドライバは複数の同時接続ハブを完全に
 * サポートしている（正確には ::MAX_NUSBHUBS 個まで）。
 */
static usb_status_t
hub_bind_device(struct usb_device *dev)
{
    usb_status_t status;
    struct usb_hub *hub;
    int hub_id;

    /* 新しいデバイス化ハブであるかチェックする  */
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

    /* ハブデータを割り当てる  */
    hub_id = hub_alloc();
    if (SYSERR == hub_id)
    {
        usb_error("Too many hubs attached.\n");
        return USB_STATUS_DEVICE_UNSUPPORTED;
    }

    hub = &hub_structs[hub_id];
    hub->device = dev;

    /* はブディクリプタを読み込む  */
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

    /* 適切な数のUSBポート構造体を初期化する  */
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

/**
 * デタッチされたハブからハブドライバをアンバインドする.  これはハブドライバ用の
 * @ref usb_device_driver::unbind_device "unbind_device" の実装であるので
 * その文書化されている動作に準じている。ただし、ハブドライバのみが対処する
 * 必要がある重要な詳細は子デバイスを再帰的にデタッチすることである。
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

    /* ハブスレッドで処理待ちのステータス変化リクエストのマスクから取り外された
     * ハブのステータス変化リクエストを削除する.  これを行うには
     * hub_status_change_pending の対応するビットをクリアする必要がある。
     * 「別の」ハブの同じビットマスクのビットを設定しようとする
     * hub_status_changed() と競合しないように割り込みを一時的に無効にする
     * 必要がある。 */
    im = disable();
    hub_status_change_pending &= ~(1 << hub_id);
    restore(im);

    /* 取り外されたハブ用のusb_hub構造体を開放する */
    hub_free(hub_id);
}

/**
 * @ingroup usbhub
 * USBハブデバイスドライバの宣言.
 */
const struct usb_device_driver usb_hub_driver = {
    .name          = "USB Hub Driver",
    .bind_device   = hub_bind_device,
    .unbind_device = hub_unbind_device,
};
