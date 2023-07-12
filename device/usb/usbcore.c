/**
 * @file     usbcore.c
 * @ingroup  usbcore
 * @ingroup  usb
 *
 * このファイルにはUSBコアドライバが含まれている.
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

/**
 * @addtogroup usbcore
 *
 * USBコアドライバである。特定のホストコントローラハードウェアに依存しない
 * USBシステムソフトウェアを実装しており、特定のUSBデバイスやプラットフォームに
 * 特化したものではない。
 *
 * 機能および制限事項
 *
 *  - このドライバはUSB2.0に準拠するように書かれている。USB3.0デバイスは未検証である。
 *
 *  - すべてのUSB転送タイプと速度に対応しているわけではない。これはホスト
 *    コントローラドライバに依存する; usb_submit_xfer_request() を参照されたい。
 *
 *  - このドライバは、インテリジェントな電力管理、帯域幅の割り当て、転送
 *    スケジューリングは行わない。デバイスは電力要件に関係なく、常に最初に
 *    リストアップされているコンフィグレーションに設定される。転送リクエストは
 *    発行された順にそのままホストコントローラデバイスに渡される。したがって、
 *    必要であればホストコントローラドライバがよりインテリジェントな
 *    スケジューリングを行う責任がある。
 *
 *  - このドライバは1つのUSB（バス）を駆動することを前提としている。複数の
 *    USBには対応していない。
 *
 *  - このドライバはUSBデバイスに対して複数のコンフィギュレーションは対応して
 *    いない。デバイスに複数のコンフィギュレーションがある場合は、最初の
 *    コンフィギュレーションが割り当てられる。
 *
 *  - このドライバは現在のところドライバをUSBインタフェースではなく、USB
 *    デバイスとしてバインディングするように書かれている。
 *
 *  - 設計上、ホストコントローラドライバが usb_hcdi.h で宣言されている
 *    関数を提供する限り、このコードを変更することなく、異なるホストコントローラ
 *    ハードウェア用のホストコントローラドライバを実装することが可能である。
 *
 *  - 設計上、このドライバはUSBハブドライバにハードコードされた依存性を持って
 *    いる。USBはハブなしでは使い物にないからである。
 *
 * このコアUSBドライバを初期化するために、システム起動コードは usbinit() を
 * 呼び出す必要がある。詳しくはその関数を参照されたい。
 *
 * このコアUSBドライバがエクスポートしている他の関数は、ほとんどがUSBデバイス
 * ドライバが使用することを想定している。
 *
 * このドライバと他のUSBコードで使われるデバッグメッセージは usb_util.h にある
 * 最小、初期、バックグラウンドのログ優先度の定義を変更することで有効にできる。
 */

#include <memory.h>
#include <stdlib.h>
#include <semaphore.h>
#include <usb_core_driver.h>
#include <usb_hcdi.h>
#include <usb_hub_driver.h>
#include <usb_std_defs.h>
#include <usb_subsystem.h>
#include "../../system/platforms/arm-rpi3/mmu.h"
#include <clock.h>

/** @ingroup usbcore
 * @def MAX_NUSBDEV
 * 同時にサポートするUSBデバイスの最大数  */
#define MAX_NUSBDEV 32

/** @ingroup usbcore
 * @def MAX_NUSBDRV
 * 同時にサポートするUSBデバイスドライバの最大数  */
#define MAX_NUSBDRV 16

/** @ingroup usbcore
 * @var usb_devices[MAX_NUSBDEV]
 * 必要に応じて実際のデバイスに動的に割り当てることができるUSB
 * デバイス構造体のテーブル */
struct usb_device usb_devices[MAX_NUSBDEV];

/** @ingroup usbcore
 * @var usb_device_drivers[MAX_NUSBDEV]
 * USBコアに登録されているUSBデバイスドライバのテーブル. */
static const struct usb_device_driver *usb_device_drivers[MAX_NUSBDRV];

/** @ingroup usbcore
 * @var usb_num_device_drivers
 * @brief 現在登録されているUSBデバイスドライバの数. 登録されたデバイスドライバを
 * 削除する方法は現在のところ存在しない。したがって、この数は増加しかしない  */
static uint usb_num_device_drivers = 0;

/** @ingroup usbcore
 * @var usb_root_hub
 * @brief ルートハブへのポインタ. USBサブシステムが初期化されていない場合はNULL */
struct usb_device *usb_root_hub = NULL;

/**
 * @ingroup usbcore
 *
 * usb_alloc_xfer_request() 以外の方法でデバイスドライバにより割り当てられた
 * USB転送リクエストに対して一度だけ初期化を行う.
 *
 * @param req
 *      初期化するUSB転送リクエストへのポインタ
 */
void
usb_init_xfer_request(struct usb_xfer_request *req)
{
    bzero(req, sizeof(struct usb_xfer_request));
    /* TODO: HCD固有の変数はもっと良い扱いをする必要がある  */
    /* これらは必要になった時点で正しい値が設定される。これは未設定の目印となる.*/
    req->deferer_thread_tid = BADTID;
    req->deferer_thread_sema = SYSERR;
}


/**
 * @ingroup usbcore
 *
 * usb_xfer_request構造体をデータバッファとともに動的に割り当てる.
 *
 * @param bufsize
 *      送受信用のデータバッファ長
 *
 * @return
 *      割り当てたusb_xfer_request構造体へのポインタ。メモリ不足の場合は NULL
 */
struct usb_xfer_request *
usb_alloc_xfer_request(uint bufsize)
{
    struct usb_xfer_request *req;

    req = memget(sizeof(struct usb_xfer_request) + bufsize);
    if (req == (void*)SYSERR)
    {
        return NULL;
    }
    usb_init_xfer_request(req);
    /* | request : buffer | */
    req->sendbuf = (uint8_t*)(req + 1);
    req->size = bufsize;
    return req;
}

/**
 * @ingroup usbcore
 *
 * usb_alloc_xfer_request() で割り当てたusb_xfer_request構造体を解放する.
 *
 * @param req
 *      解放するusb_xfer_request構造体へのポインタ。現在保留中であっては
 *      ならない。No-opとしてNULLを渡すことができる。
 */
void usb_free_xfer_request(struct usb_xfer_request *req)
{
    if (req != NULL)
    {
        /* TODO: HCD-specific variables need to be handled better.  */
        kill(req->deferer_thread_tid);
        semfree(req->deferer_thread_sema);
        memfree(req, sizeof(struct usb_xfer_request) + req->size);
    }
}

/**
 * @ingroup usbcore
 *
 * USB転送の実行を要求する.
 *
 * この関数はホストコントローラが要求を後で処理するように、転送リクエストを
 * キューに入れるだけの非同期インタフェースであることを意図している。この関数は
 * 直ちに復帰し、転送要求はその後、CPUとホストコントローラ間の割り込みを
 * 利用して効率的に行う必要がある。しかし、どのように転送要求を完了させるかは
 * 最終的にはホストコントローラドライバに委ねられる。同期処理も可能だが推奨はしない。
 *
 * 転送が完了または失敗すると @p req の @ref
 * usb_xfer_request::completion_cb_func "completion_cb_func" 関数が呼び出される。
 * このコールバック関数では完了または失敗したリクエストを処理し、そのメモリを
 * 解放するか、次の転送を開始するための要求を再発行する必要がある。
 *
 * 必ずしもすべてのUSB転送タイプと速度がサポートされているわけではない。
 * これはホストコントローラドライバ次第である。ホストコントローラは渡された
 * 転送要求がサポートしているタイプと速度であるかを判断し、していない場合は
 * ::USB_STATUS_INVALID_PARAMETER を返すことが期待されている。現在のところ
 * （Raspberry Piに搭載されている）Synopsys DesignWare High-Speed USB 2.0
 * On-The-Go ControllerのホストコントローラドライバはLS,FS,HSのコントロール
 * 転送、バルク転送、インターラプト転送に対応する予定であるが、まだ実際に
 * すべての転送タイプと速度の組み合わせがテストされたわけではない。
 *
 * @param req
 *      発呼するUSB転送要求へのポインタ.  入力として定義されている
 *      メンバーにはデータが設定されている必要がある。
 *
 * @retval ::USB_STATUS_SUCCESS
 *      要求の発行が成功した.
 * @retval ::USB_STATUS_INVALID_PARAMETER
 *      @p req が正しく設定されていなかった.
 * @retval ::USB_STATUS_DEVICE_DETACHED
 *      デバイスが取り外されていた.
 */
usb_status_t
usb_submit_xfer_request(struct usb_xfer_request *req)
{
#if USB_MIN_LOG_PRIORITY <= USB_LOG_PRIORITY_DEBUG
    enum usb_transfer_type type;
    enum usb_direction dir;
#endif
    irqmask im;
    usb_status_t status;

    /* 引数チェック */
    if (!req || !req->dev || !req->completion_cb_func ||
        !req->dev->inuse)
    {
        usb_error("Bad usb_xfer_request: no device or completion callback "
                  "function\n");
        return USB_STATUS_INVALID_PARAMETER;
    }

    im = disable();

    /* 取り外し中のデバイスに新たなリクエストを発行することはできない */
    if (req->dev->state == USB_DEVICE_DETACHMENT_PENDING)
    {
        usb_dev_debug(req->dev, "Device detachment pending; "
                      "refusing new xfer\n");
        restore(im);
        return USB_STATUS_DEVICE_DETACHED;
    }

#if USB_MIN_LOG_PRIORITY <= USB_LOG_PRIORITY_DEBUG
    if (req->endpoint_desc)
    {
        type = req->endpoint_desc->bmAttributes & 0x3;
        dir = req->endpoint_desc->bEndpointAddress >> 7;
    }
    else
    {
        type = USB_TRANSFER_TYPE_CONTROL;
        dir = req->setup_data.bmRequestType >> 7;
    }
    usb_dev_debug(req->dev, "Submitting xfer request (%u bytes, "
                  "type=%s, dir=%s)\n",
                  req->size,
                  usb_transfer_type_to_string(type),
                  usb_direction_to_string(dir));
    if (type == USB_TRANSFER_TYPE_CONTROL)
    {
        usb_dev_debug(req->dev, "Control message: {.bmRequestType=0x%02x, "
                      ".bRequest=0x%02x, wValue=0x%04x, wIndex=0x%04x, wLength=0x%04x}\n",
                      req->setup_data.bmRequestType,
                      req->setup_data.bRequest,
                      req->setup_data.wValue,
                      req->setup_data.wIndex,
                      req->setup_data.wLength);
    }
#endif
    req->status = USB_STATUS_NOT_PROCESSED;
    req->actual_size = 0;
    req->complete_split = 0;
    req->control_phase = 0;
    ++req->dev->xfer_pending_count;
    /* HCDに転送要求 */
    status = hcd_submit_xfer_request(req);
    if (status != USB_STATUS_SUCCESS)
    {
        --req->dev->xfer_pending_count;
    }
    restore(im);
    return status;
}

/**
 * @ingroup usbcore
 *
 * USB転送が正常に完了した、またはエラーが発生したことをデバイスドライバに
 * 通知する.  転送が完了した際にホストコントローラドライバから呼び出される。
 *
 * @param req
 *        完了を通知するUSB転送
 */
void
usb_complete_xfer(struct usb_xfer_request *req)
{
    irqmask im;

    im = disable();

    --req->dev->xfer_pending_count;

    /* デバイスが取り外し保留の場合は転送状態を上書きする */
    if (req->dev->state == USB_DEVICE_DETACHMENT_PENDING)
    {
        req->status = USB_STATUS_DEVICE_DETACHED;
    }

    usb_dev_debug(req->dev,
                  "Calling completion callback (Actual transfer size %u "
                  "of %u bytes, type=%s, dir=%s, status=%d)\n",
                  req->actual_size, req->size,
                  usb_transfer_type_to_string(
                        req->endpoint_desc ?
                            (req->endpoint_desc->bmAttributes & 0x3) :
                            USB_TRANSFER_TYPE_CONTROL),
                  usb_direction_to_string(
                        req->endpoint_desc ?
                            (req->endpoint_desc->bEndpointAddress >> 7) :
                            req->setup_data.bmRequestType >> 7),
                  req->status);

    /* 成功しなかったらエラー情報を更新する */
    if (req->status != USB_STATUS_SUCCESS)
    {
        req->dev->error_count++;
        req->dev->last_error = req->status;
    }

    /* 完了コールバック関数を実際に呼び出す  */
    (*req->completion_cb_func)(req);

    /* デバイスが取り外し中で、最後の保留中の転送を完了させた場合は
     * usb_free_device() で待機中のスレッドに通知する */
    if (req->dev->state == USB_DEVICE_DETACHMENT_PENDING &&
        req->dev->xfer_pending_count == 0)
    {
        send(req->dev->quiescent_state_waiter, 0);
    }

    restore(im);
}

/** @ingourp usbcode
 * コントロール転送が完了したことを通知する.
 * コントロール転送の場合の完了コールバック関数として使用し、非同期の転送処理を
 * 同期的に使用する。
 * @param req USB転送要求へのポインタ
 */
static void
signal_control_msg_done(struct usb_xfer_request *req)
{
    /* privateにはセマフォが設定されており、このセマフォでwaitしている */
    signal((semaphore)req->private);
}

/**
 * @ingroup usbcore
 *
 * USBデバイスとの間でコントロール転送を同期的に実行する.  コントロール
 * メッセージはUSBの4つの基本的な転送タイプの1つであり、USB 2.0仕様の
 * 様々な場所で文書化されている。
 *
 * これは同期インタフェースなので、コントロール転送が完了するか、タイム
 * アウトするか、別のエラーが発生するまでスレッドはブロックされる。
 * 現在、リクエストタイムアウトはホストコントローラドライバにより
 * 選択されており、パラメータとして渡すことはできない。
 *
 * @param dev
 *      コントロールメッセージを発行するUSBデバイスへのポインタ
 * @param endpoint_desc
 *      コントロール転送を発行するエンドポイントのエンドポイント
 *      ディスクリプタへのポインタ。または、デフォルトコントロール
 *      ディスクリプタ（エンドポイントを持たない）を指定する場合は
 *      NULL
 * @param bRequest
 *      リクエスト. 標準デバイスリクエストはUSB 2.0仕様の
 *      セクション 9.4に記載されている。このコードではリクエストは
 *      enum ::usb_device_request の値である。クラス固有のリクエスト
 *      など、その他の値もこのパラメタとして使用することができる、
 * @param bmRequestType
 *      リクエストタイプ. 標準デバイスリクエストはUSB 2.0仕様の
 *      セクション 9.3.1に記載されている。このコードではリクエストは
 *      enum ::usb_bmRequestType_fields の値である。
 * @param wValue
 *      リクエスト固有のデータ
 * @param wIndex
 *      リクエスト固有のデータ
 * @param data
 *      バッファへのポインタ。バッファはリクエストでメッセージの一部として
 *      送信される追加データを保持、または、USBデバイスから受信した追加データを
 *      受信する。 @p wLength が 0 の場合、このパラメタは無視される。
 * @param wLength
 *      @p data バッファ長.  この値は転送されなければならない追加データの正確な
 *      バイト数と解釈される。リクエストを満たすために追加データを転送
 *      する必要がない場合は 0 にすることができる。たとえば、SetAddress
 *      リクエストは情報をwValueで渡すので追加のデータバッファは必要としない。
 *
 * @return
 *      転送のステータス。返り値の候補は以下のいずれかである。
 *
 * @retval ::USB_STATUS_SUCCESS
 *      @p wLength で指定された量のデータの転送が成功した。
 * @retval ::USB_STATUS_OUT_OF_MEMORY
 *      メモリの割り当て、または、セマフォの作成に失敗した。
 * @retval ::USB_STATUS_INVALID_DATA
 *      転送は完了したが、指定されただけのデータは転送されなかった。
 * @retval ::USB_STATUS_UNSUPPORTED_REQUEST
 *      転送はルートハブへのものだったがそのリクエストは実装されていなかった。
 * @retval ::USB_STATUS_HARDWARE_ERROR
 *      ハードウェアエラーが発生した。
 * @retval ::USB_STATUS_DEVICE_DETACHED
 *      USBデバイスが取り外されていた。
 */
usb_status_t
usb_control_msg(struct usb_device *dev,
                const struct usb_endpoint_descriptor *endpoint_desc,
                uint8_t bRequest, uint8_t bmRequestType,
                uint16_t wValue, uint16_t wIndex, void *data, uint16_t wLength)
{
    usb_status_t status;
    struct usb_xfer_request *req;
    semaphore sem;

    /* 同期操作用のセマフォを作成する */
    sem = semcreate(0);
    if (isbadsem(sem))
    {
        return USB_STATUS_OUT_OF_MEMORY;
    }
    /* USB転送要求を作成する */
    req = usb_alloc_xfer_request(wLength);
    if (req == NULL)
    {
        semfree(sem);
        return USB_STATUS_OUT_OF_MEMORY;
    }
    req->dev = dev;
    req->endpoint_desc = endpoint_desc;
    req->recvbuf = data;
    req->size = wLength;
    req->setup_data.bmRequestType = bmRequestType;
    req->setup_data.bRequest = bRequest;
    req->setup_data.wValue = wValue;
    req->setup_data.wIndex = wIndex;
    req->setup_data.wLength = wLength;
    req->completion_cb_func = signal_control_msg_done;
    req->private = (void*)sem;      // privateデータにセマフォを設定。
    /* 転送実行を要求 */
    status = usb_submit_xfer_request(req);
    if (status == USB_STATUS_SUCCESS)
    {
        /* 転送が完了（または失敗）するまで待機する。
         * usb_submit_xfer_requestは非同期関数ですぐに復帰する。
         * 転送が完了するとコールバック関数が呼ばれ、その中でsignalされる */
        wait(sem);
        status = req->status;

        /* 実際の転送サイズが依頼したサイズと違う場合はエラーとする  */
        if (status == USB_STATUS_SUCCESS && req->actual_size != req->size)
        {
            status = USB_STATUS_INVALID_DATA;
            req->dev->error_count++;
            req->dev->last_error = status;
        }
    }
    /* 転送要求を解放する */
    usb_free_xfer_request(req);
    /* セマフォを解放する */
    semfree(sem);
    return status;
}

/**
 * @ingroup usbcore
 *
 * USBデバイスからディスクリプタを読み込む. これは usb_control_msg() を
 * ラップしたものであり、ディスクリプタヘッダーの読み込みと適切な長さの
 * リクエストを自動的に処理する（まず、ショート形式で読み込み、必要な
 * 長さを取得して、完全形式で読み取ることを1つの関数で行う）
 *
 * @param dev
 *      ディスクリプタを読み込むUSB device
 * @param bRequest
 *      usb_control_msg()を参照
 * @param bmRequestType
 *      usb_control_msg()を参照
 * @param wValue
 *      usb_control_msg()を参照
 * @param wIndex
 *      usb_control_msg()を参照
 * @param buf
 *      usb_control_msg()を参照
 * @param buflen
 *      usb_control_msg()を参照
 *
 * @return
 *      usb_control_msg() と同じ返り値。 ディスクリプタのbLength
 *      フィールドがディスクリプタヘッダーのサイズより小さい場合は
 *      ::USB_STATUS_INVALID_DATA
 */
usb_status_t
usb_get_descriptor(struct usb_device *dev, uint8_t bRequest, uint8_t bmRequestType,
                   uint16_t wValue, uint16_t wIndex, void *buf, uint16_t buflen)
{
    usb_status_t status;
    uint16_t len;

    if (buflen > sizeof(struct usb_descriptor_header))
    {
        /* 実際のディスクリプタ長を取得する */
        struct usb_descriptor_header hdr;

        status = usb_control_msg(dev, NULL, bRequest, bmRequestType,
                                 wValue, wIndex, &hdr, sizeof(hdr));
        if (status != USB_STATUS_SUCCESS)
        {
            return status;
        }

        if (hdr.bLength < sizeof(hdr))
        {
            usb_dev_error(dev, "Descriptor length too short\n");
            return USB_STATUS_INVALID_DATA;
        }

        /* 読み込むべきデータ長は、実際のディスクリプタ長かバッファ長の小さい方  */
        len = min(hdr.bLength, buflen);
    }
    else
    {
        len = buflen;
    }
    /* 正式なディクリプタを読み込む */
    return usb_control_msg(dev, NULL, bRequest, bmRequestType, wValue, wIndex,
                           buf, len);
}

/** @ingroup usbcore
 *
 * USBデバイスのデバイスディスクリプタを読み込むもう1つの方法.
 * 長さを指定してデバイスディスクリプタまたはそのヘッダー部分を
 * dev->descriptorに直接読み込む。
 *
 * @param dev USBデバイス
 * @param maxlen 読み込むデータ長
 * @return usb_control_msg() が返す任意の値。
 * */
static usb_status_t
usb_read_device_descriptor(struct usb_device *dev, uint16_t maxlen)
{
    /* 注: デバイスディスクリプタの最小の長さを超えて読み込むことはないので
     * ここでは usb_get_descriptor() を使用する必要はない。 */
    return usb_control_msg(dev, NULL,
                           USB_DEVICE_REQUEST_GET_DESCRIPTOR,
                           USB_BMREQUESTTYPE_DIR_IN |
                               USB_BMREQUESTTYPE_TYPE_STANDARD |
                               USB_BMREQUESTTYPE_RECIPIENT_DEVICE,
                           USB_DESCRIPTOR_TYPE_DEVICE << 8, 0,
                           &dev->descriptor, maxlen);
}

/** @ingroup usbcore
 * USBデバイスから指定したコンフィグレーションディスクリプタ、または、その
 * ヘッダ部分をバッファに読み込む.
 *
 * @param dev USBデバイス
 * @param configuration_idx コンフィグレーションディスクリプタのインデックス
 * @param buf ディスクリプタを書き込むバッファ
 * @param buflen バッファ長（ディスクリプタかヘッダ部分かを識別する）
 * @return usb_control_msg() が返す任意の値。
 *  */
static usb_status_t
usb_get_configuration_descriptor(struct usb_device *dev, uint8_t configuration_idx,
                                 void *buf, uint16_t buflen)
{
    return usb_control_msg(dev, NULL, USB_DEVICE_REQUEST_GET_DESCRIPTOR,
                           USB_BMREQUESTTYPE_DIR_IN |
                               USB_BMREQUESTTYPE_TYPE_STANDARD |
                               USB_BMREQUESTTYPE_RECIPIENT_DEVICE,
                           (USB_DESCRIPTOR_TYPE_CONFIGURATION << 8) | configuration_idx,
                           0, buf, buflen);
}

/** @ingroup usbcore
 *
 * USBデバイスの最初のコンフィグレーションディスクリプタを動的に割り当てた
 * dev->config_descriptor バッファに読み込む. このバッファは初期には
 * 未割り当てであると仮定している。
 *
 * @param dev
 *      コンフィグレーションディスクリプタを読み込むデバイスのUSBデバイス
 *      構造体へのポインタ
 * @param configuration
 *      読み込むコンフィグレーションのインデックス
 *
 * @return
 *      コンフィグレーションディスクリプタ用のメモリが割り当てられなかった
 *      場合は ::USB_STATUS_OUT_OF_MEMORY、コンフィグレーションディスクリプタが
 *      不正だった場合は ::USB_STATUS_INVALID_PARAMETER。それ以外は
 *      usb_control_msg() が返す任意の値。
 */
static usb_status_t
usb_read_configuration_descriptor(struct usb_device *dev, uint8_t configuration)
{
    struct usb_configuration_descriptor desc;
    usb_status_t status;
    int interface_idx;
    int endpoint_idx;
    uint i;
    struct usb_descriptor_header *hdr;
    bool in_alternate_setting;

    /* コンフィグレーションディスクリプタのサイズを取得する */
    /* config descriptorのみ取得 */
    status = usb_get_configuration_descriptor(dev, configuration,
                                              &desc, sizeof(desc));
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }

    /* 完全なコンフィグレーションディスクリプタ用のバッファを割り当てる */
    dev->config_descriptor = memget(desc.wTotalLength);
    if (dev->config_descriptor == (void*)SYSERR)
    {
        return USB_STATUS_OUT_OF_MEMORY;
    }

    /* 実際のコンフィグレーションディスクリプタを取得する */
    /* config, interface, endpoint, class-specific, vendor-specificを含む */
    status = usb_get_configuration_descriptor(dev, configuration,
                                              dev->config_descriptor,
                                              desc.wTotalLength);
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }

    /* インタフェースディスクリプタとエンドポイントディスクリプタを
     * 取り出してポインタをセットする
     */
    interface_idx = -1;
    endpoint_idx = -1;
    in_alternate_setting = FALSE;
    for (i = 0;
         i + sizeof(struct usb_descriptor_header) <= desc.wTotalLength;
         i += hdr->bLength)
    {
        hdr = (struct usb_descriptor_header*)((uint8_t*)dev->config_descriptor + i);

        if (hdr->bLength < sizeof(struct usb_descriptor_header))
        {
            goto out_invalid;
        }
        /* インタフェースディスクリプタとエンドポイントディスクリプタを取り出す */
        switch (hdr->bDescriptorType)
        {
            case USB_DESCRIPTOR_TYPE_INTERFACE:
                if (i + sizeof(struct usb_interface_descriptor) > desc.wTotalLength)
                {
                    goto out_invalid;
                }
                if (interface_idx >= 0 && !in_alternate_setting &&
                    endpoint_idx + 1 !=
                            dev->interfaces[interface_idx]->bNumEndpoints)
                {
                    usb_dev_debug(dev, "Number of endpoints incorrect\n");
                    goto out_invalid;
                }
                if (((struct usb_interface_descriptor*)hdr)->bAlternateSetting != 0)
                {
                    in_alternate_setting = TRUE;
                }
                else
                {
                    in_alternate_setting = FALSE;
                    if (++interface_idx >= USB_DEVICE_MAX_INTERFACES)
                    {
                        usb_dev_error(dev,
                                      "Too many interfaces (this driver only "
                                      "supports %u per configuration)\n",
                                      USB_DEVICE_MAX_INTERFACES);
                        return USB_STATUS_DEVICE_UNSUPPORTED;
                    }
                    dev->interfaces[interface_idx] =
                                (struct usb_interface_descriptor*)hdr;
                    endpoint_idx = -1;
                }
                break;
            case USB_DESCRIPTOR_TYPE_ENDPOINT:
                if (interface_idx < 0)
                {
                    goto out_invalid;
                }
                if (i + sizeof(struct usb_endpoint_descriptor) > desc.wTotalLength)
                {
                    goto out_invalid;
                }
                if (!in_alternate_setting)
                {
                    if (++endpoint_idx >= USB_DEVICE_MAX_ENDPOINTS)
                    {
                        usb_dev_error(dev,
                                      "Too many endpoints (this driver only "
                                      "supports %u per interface)\n",
                                      USB_DEVICE_MAX_ENDPOINTS);
                        return USB_STATUS_DEVICE_UNSUPPORTED;
                    }
                    dev->endpoints[interface_idx][endpoint_idx] =
                                (struct usb_endpoint_descriptor*)hdr;
                }
                break;
            default:
                break;
        }
    }
    if (interface_idx + 1 != dev->config_descriptor->bNumInterfaces)
    {
        usb_dev_debug(dev, "Number of interfaces incorrect (interface_idx=%d)\n",
                      interface_idx);
        goto out_invalid;
    }

    return USB_STATUS_SUCCESS;
out_invalid:
    usb_dev_error(dev, "Configuration descriptor invalid\n");
    return USB_STATUS_INVALID_DATA;
}

/** @ingroup usbcore
 * USBデバイスのバスアドレスをセットする */
static usb_status_t
usb_set_address(struct usb_device *dev, uint8_t address)
{
    usb_status_t status;
    status = usb_control_msg(dev, NULL, /* default control endpoint */
                             USB_DEVICE_REQUEST_SET_ADDRESS,
                             USB_BMREQUESTTYPE_DIR_OUT |
                                USB_BMREQUESTTYPE_TYPE_STANDARD |
                                USB_BMREQUESTTYPE_RECIPIENT_DEVICE,
                             address, /* wValue: address */
                             0, NULL, 0); /* wIndex, buf, wLength: unused */
    if (status == USB_STATUS_SUCCESS)
    {
        dev->address = address;
    }
    return status;
}

/** @ingroup usbcore
 * USBデバイスを指定のconfigurationで構成する.
 */
static usb_status_t
usb_set_configuration(struct usb_device *dev, uint8_t configuration)
{
    usb_status_t status;

    status = usb_control_msg(dev,
                             NULL, /* endpoint 0 */
                             USB_DEVICE_REQUEST_SET_CONFIGURATION,
                             USB_BMREQUESTTYPE_DIR_OUT |
                                USB_BMREQUESTTYPE_TYPE_STANDARD |
                                USB_BMREQUESTTYPE_RECIPIENT_DEVICE,
                             configuration, /* wValue: configuration index */
                             0, NULL, 0); /* wIndex, data, wLength: unused */
    if (status == USB_STATUS_SUCCESS)
    {
        dev->configuration = configuration;
    }
    return status;
}


/**
 * @ingroup usbcore
 *
 * デバイス構造体のプールから利用可能なUSBデバイス構造体を割り当てる.
 *
 * @param parent
 *      親ハブのUSBデバイス構造体へのポインタ。割り当てるデバイスが
 *      ルートハブの場合は @c NULL

 * @return
 *      初期化されたUSBデバイス構造体へのポインタ。USBデバイス構造体が
 *      割り当てられなかった場合は @c NULL
 */
struct usb_device *
usb_alloc_device(struct usb_device *parent)
{
    uint i;
    struct usb_device *dev;
    irqmask im;

    dev = NULL;
    im = disable();
    for (i = 0; i < MAX_NUSBDEV; i++)
    {
        /* 未使用のデバイス構造体があれば割り当てる */
        if (!usb_devices[i].inuse)
        {
            dev = &usb_devices[i];
            bzero(dev, sizeof(struct usb_device));
            dev->inuse = TRUE;              /* 使用済みとする */
            dev->speed = USB_SPEED_HIGH;    /* デフォルトはHS */
            dev->parent = parent;
            if (parent != NULL)
            {
                dev->depth = parent->depth + 1;
            }
            dev->last_error = USB_STATUS_SUCCESS;
            dev->state = USB_DEVICE_ATTACHED;
            dev->quiescent_state_waiter = BADTID;
            break;
        }
    }
    restore(im);
    return dev;
}

/**
 * @ingroup usbcore
 *
 * USBデバイスを取り外し解放する.  デバイスドライバにバインドされていた
 * 場合はアンバインドする。そして、USBデバイス構造体をデバイス構造体プールに
 * 返す。ハブドライバだけがこの関数を呼び出す必要がある。
 *
 * @param dev
 *      取り外すUSBデバイス構造体へのポインタ
 */
void
usb_free_device(struct usb_device *dev)
{
    irqmask im;

    /* このデバイスへの新たな転送を不許可とし、保留中のすべての転送が
     * 完了するのを待つ  */
    im = disable();
    dev->state = USB_DEVICE_DETACHMENT_PENDING;
    if (dev->xfer_pending_count != 0)
    {
        usb_dev_debug(dev, "Waiting for %u pending xfers to complete\n",
                      dev->xfer_pending_count);
        dev->quiescent_state_waiter = gettid();
        receive();
    }
    restore(im);

    /* 必要であればデバイスドライバをアンバインドする */
    if (dev->driver != NULL && dev->driver->unbind_device != NULL)
    {
        usb_dev_debug(dev, "Unbinding %s\n", dev->driver->name);
        dev->driver->unbind_device(dev);
    }

    /* コンフィグレーションディスクリプタとデバイス構造体を解放する */
    usb_dev_debug(dev, "Releasing USB device structure.\n");
    if (dev->config_descriptor != NULL)
    {
        memfree(dev->config_descriptor, dev->config_descriptor->wTotalLength);
    }
    dev->inuse = FALSE;
}

/**
 * @ingroup usbcore
 *
 * USBデバイスドライバをUSBデバイスにバインドする.
 *
 * @param dev
 *      ドライバをバインドする、新しくアドレス付けされ構成されたUSBデバイス
 *
 * @return
 *      ドライバのバインドが成功した、または、すでにバインドされていた場合は
 *      ::USB_STATUS_SUCCESS; デバイスをサポートしているドライバが存在しない
 *      場合は ::USB_STATUS_DEVICE_UNSUPPORTED; それ以外はドライバの初期化関数
 *      が返したエラーコード
 */
static usb_status_t
usb_try_to_bind_device_driver(struct usb_device *dev)
{
    usb_status_t status;
    uint i;

    if (dev->driver != NULL)
    {
        /* ドライバがすでにバインドされているは成功で復帰  */
        return USB_STATUS_SUCCESS;
    }

    status = USB_STATUS_DEVICE_UNSUPPORTED;
    for (i = 0; i < usb_num_device_drivers; i++)
    {
        usb_dev_debug(dev, "Attempting to bind %s to device\n",
                      usb_device_drivers[i]->name);
        /* とりあえずバインドしてみて、成功したらOK */
        status = usb_device_drivers[i]->bind_device(dev);
        if (status != USB_STATUS_DEVICE_UNSUPPORTED)
        {
            if (status == USB_STATUS_SUCCESS)
            {
                dev->driver = usb_device_drivers[i];
                usb_info("Bound %s to %s\n", dev->driver->name,
                         usb_device_description(dev));
            }
            break;
        }
    }
    return status;
}

/**
 * @ingroup usbcore
 *
 * 新しく割り当てられたUSBデバイスを構成・初期化（すなわち「エヌメレート」）する.
 * 物理デバイスは初期状態ではアドレス設定も構成もされていないため、デフォルト
 * アドレス0にコントールメッセージを送信することでアクセスできると仮定する。
 *
 * @param dev
 *      構成と初期化を行う新しいUSBデバイス.
 * @return
 *      成功した場合は ::USB_STATUS_SUCCESS ; それ以外は ::usb_status_t
 *      型のエラーコード。成功が返されても、新しく接続されたデバイが実際の
 *      デバイスドライバにバインドされている場合もあれば、されていない場合も
 *      あることに注意されたい（デバイスdライバがない場合も成功としているため）。
 */
usb_status_t
usb_attach_device(struct usb_device *dev)
{
    usb_status_t status;
    uint8_t address;

    /* コントロール転送を使用してUSBデバイスと通信するためには、デバイスが
     * サポートする最大パケットサイズを知る必要がある。これは自明ではない。
     * なぜなら、最大パケットサイズはデバイスディスクリプタに格納されているが、
     * それ自体、コントール転送で読み込む必要があるためである。この問題を
     * 回避するため、USB2.0仕様に従って、USBシステムソフトウェアは最初の
     * パケットでデバイスディスクリプタの先頭の8バイトだけを読み取る必要が
     * ある。これには以降の転送で使用するべき最大パケットサイズが含まれている。
     * 最大パケットサイズは少なくとも8バイトであることが保証されているため
     * これでうまくいく。 */
    usb_dev_debug(dev, "Getting maximum packet size from start of "
                  "device descriptor\n");
    dev->descriptor.bMaxPacketSize0 = 8;
    status = usb_read_device_descriptor(dev, 8);
    if (status != USB_STATUS_SUCCESS)
    {
        usb_dev_error(dev, "Failed to read start of device descriptor: %s\n",
                      usb_status_string(status));
        return status;
    }

    usb_dev_debug(dev, "Using bMaxPacketSize0=%u\n", dev->descriptor.bMaxPacketSize0);

    /* このデバイスにアドレスを付与する。ユニークなアドレスを得るために、
     * ここでは usb_devices配列内のこの `struct usb_device' の1始まりの
     * インデックスを使用する。 */
    address = (dev - usb_devices) + 1;
    usb_dev_debug(dev, "Assigning address %u to new device\n", address);
    status = usb_set_address(dev, address);
    if (status != USB_STATUS_SUCCESS)
    {
        usb_dev_error(dev, "Failed to assign address: %s\n",
                      usb_status_string(status));
        return status;
    }

    /* このデバイスに関する情報を取得するためにデバイスディスクリプタを読み込む */
    usb_debug("Reading device descriptor.\n");
    status = usb_read_device_descriptor(dev, sizeof(dev->descriptor));
    if (status != USB_STATUS_SUCCESS)
    {
        usb_dev_error(dev, "Failed to read device descriptor: %s\n",
                      usb_status_string(status));
        return status;
    }

#if !USB_EMBEDDED
    /* もしあれば、商品名と製造者名を読み込む  */
    if (dev->descriptor.iProduct != 0)
    {
        usb_debug("Reading product string.\n");
        usb_get_ascii_string(dev, dev->descriptor.iProduct,
                             dev->product, sizeof(dev->product));
    }
    if (dev->descriptor.iManufacturer != 0)
    {
        usb_debug("Reading manufacturer string.\n");
        usb_get_ascii_string(dev, dev->descriptor.iManufacturer,
                             dev->manufacturer, sizeof(dev->manufacturer));
    }
#endif

    /* 最初のコンフィグレーションディスクリプタを読み込む  */
    usb_debug("Reading configuration descriptor.\n");
    status = usb_read_configuration_descriptor(dev, 0);
    if (status != USB_STATUS_SUCCESS)
    {
        usb_dev_error(dev, "Failed to read configuration descriptor: %s\n",
                      usb_status_string(status));
        return status;
    }

    /* 最初に報告されたコンフィグレーションでデバイスを構成する  */
    usb_dev_debug(dev, "Assigning configuration %u (%u interfaces available)\n",
                  dev->config_descriptor->bConfigurationValue,
                  dev->config_descriptor->bNumInterfaces);
    status = usb_set_configuration(dev,
                                   dev->config_descriptor->bConfigurationValue);
    if (status != USB_STATUS_SUCCESS)
    {
        usb_dev_error(dev, "Failed to set device configuration: %s\n",
                      usb_status_string(status));
        return status;
    }

    /* Report the device attachment at an informational log level.  */
    usb_info("Attaching %s\n", usb_device_description(dev));

    /* 新しく構成されたデバイスにドライバのバインドを試みる */
    status = usb_try_to_bind_device_driver(dev);

    // TODO: ここで待たないと実行が進まない。なぜか？
    udelay(12);

    if (status == USB_STATUS_DEVICE_UNSUPPORTED)
    {
        usb_dev_info(dev, "No driver found for device.\n");
        /* 新しいデバイスをサポートするドライバは現在登録されていない。
         * しかし、デバイスを接続するためにはこれを失敗と考えるべきでは
         * ない。必要なドライバはまだ登録されていないだけかもしれない
         * からだ。  */
        status = USB_STATUS_SUCCESS;
    }
    else if (status != USB_STATUS_SUCCESS)
    {
        usb_dev_error(dev, "Failed to bind driver to new USB device: %s\n",
                      usb_status_string(status));
    }
    return status;
}

/** @ingroup usbcore
 * @var usb_bus_lock
 * @brief USBデバイスの脱着を防ぐためのセマフォ
*/
static semaphore usb_bus_lock;

/**
 * @ingroup usbcore
 *
 * USBへのデバイスの着脱を防ぐ.  これはバスに発行される
 * USB転送要求を防ぐものではない。
 */
void usb_lock_bus(void)
{
    wait(usb_bus_lock);
}

/**
 * @ingroup usbcore
 *
 * usb_lock_bus() を取り消し、USBへのデバイスの着脱を可能にする.
 */
void usb_unlock_bus(void)
{
    signal(usb_bus_lock);
}

/**
 * @ingroup usbcore
 *
 * USBデバイスドライバをUSBコアに登録する. これが呼び出された後は
 * USBコアはいつでも @ref usb_device_driver::bind_device "bind_device"
 * コールバックを実行して、デバイスドライバをUSBデバイスにバインドする
 * ことができる。
 *
 * 現在では、この関数は usbinit() の前に安全に呼び出すことができる。
 *
 * @param drv
 *      登録するUSｂデバイスドライバ構造体へのポインタ
 *
 * @return
 *      ドライバの登録に成功した場合は ::USB_STATUS_SUCCESS; それ以外は
 *      ::usb_status_t 型のエラーコード。
 */
usb_status_t
usb_register_device_driver(const struct usb_device_driver *drv)
{
    irqmask im;
    usb_status_t status;

    /* バインド用の関数が定義されている必要がある */
    if (NULL == drv->bind_device)
    {
        usb_error("bind_device function must be implemented\n");
        return USB_STATUS_INVALID_PARAMETER;
    }

    im = disable();
    /* 登録できるデバイスドライバの上限はMAX_NUSBDRV */
    if (usb_num_device_drivers >= MAX_NUSBDRV)
    {
        usb_error("Can't register new USB device driver: "
                  "too many drivers already registered\n");
        status = USB_STATUS_UNSUPPORTED_REQUEST;
    }
    else
    {
        bool already_registered = FALSE;
        uint i;
        /* 指定のドライバが登録済みでないかチェックする
         * 登録済みであれば、何もせず成功とする */
        for (i = 0; i < usb_num_device_drivers; i++)
        {
            if (drv == usb_device_drivers[i])
            {
                already_registered = TRUE;
                break;
            }
        }
        /* 未登録の場合は登録して、接続済みのデバイスにバインドする */
        if (!already_registered)
        {
            usb_device_drivers[usb_num_device_drivers++] = drv;
            usb_info("Registered %s\n", drv->name);
            /* このドライバと互換性のあるデバイスがすでにバス上にあるか
             * チェックする（ドライバの登録前に接続されたデバイスに
             * ドライバをバインドする）  */
            usb_hub_for_device_in_tree(usb_root_hub,
                                       usb_try_to_bind_device_driver);
        }
        status = USB_STATUS_SUCCESS;
    }
    restore(im);
    return status;
}

/**
 * @ingroup usb
 *
 * USBサブシステムと対応するUSBを初期化し、USBデバイスのエヌメレーション
 * 処理を開始する. USBは動的なバスであるため、エヌメレーション処理は割り込み
 * 駆動で行われ、この関数が復帰した後も継続する。したがって、この関数が復帰
 * した後でも、あるデバイスがある特定の時刻にエヌメレートされている保証はない。
 * これはUSBの設計上必要である。USBデバイスドライバはデバイスが見つかると
 * バインドされる。デバイスドライバの登録には usb_register_device_driver() を
 * 使用し、いつでも（この関数を呼ぶ前でも後でも）行うことができる。
 *
 * @return
 *      成功した場合は OK; USBサブシステムの初期化に失敗した場合は SYSERR
 */
syscall usbinit(void)
{
    struct usb_device *root_hub;
    usb_status_t status;

    /* バスをロックするためのセマフォを作成する */
    usb_bus_lock = semcreate(0);
    if (isbadsem(usb_bus_lock))
    {
        goto err;
    }
    /* USBハブドライバを登録する （usb_hub_driverはdevice/usb/usbhub.cで宣言されている）*/
    status = usb_register_device_driver(&usb_hub_driver);
    if (status != USB_STATUS_SUCCESS)
    {
        goto err_free_usb_bus_lock;
    }
    /* USbホストコントローラを起動する */
    status = hcd_start();
    if (status != USB_STATUS_SUCCESS)
    {
        usb_error("Failed to start USB host controller: %s\n",
                  usb_status_string(status));
        goto err_free_usb_bus_lock;
    }
    usb_debug("Successfully started USB host controller\n");

    /* ルートハブ（仮）を割り当てる */
    root_hub = usb_alloc_device(NULL);

    /* ルートハブ（仮）を接続する */
    usb_debug("Attaching root hub\n");
    status = usb_attach_device(root_hub);
    if (status != USB_STATUS_SUCCESS)
    {
        usb_error("Failed to attach root hub: %s\n", usb_status_string(status));
        goto err_free_root_hub;
    }
    /* ドライバをバインドしたルートハブ（仮）をUSBサブシステムの
     * ルートハブとする */
    usb_root_hub = root_hub;
    usb_debug("Successfully initialized USB subsystem\n");
    /* バスをアンロックする: TODO: どこでロックしている? */
    usb_unlock_bus();
    return OK;

err_free_root_hub:
    usb_free_device(root_hub);
    hcd_stop();
err_free_usb_bus_lock:
    semfree(usb_bus_lock);
err:
    return SYSERR;
}
