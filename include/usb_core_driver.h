/**
 * @file usb_core_driver.h
 * @ingroup usbcore
 *
 * コアUSBドライバへのインタフェース。これはUSBデバイスドライバと
 * 一部ホストコントローラドライバが使用することを意図している。
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#ifndef _USB_CORE_DRIVER_H_
#define _USB_CORE_DRIVER_H_

#include <semaphore.h>
#include <thread.h>
#include <usb_std_defs.h>
#include <usb_util.h>

extern struct usb_device *usb_root_hub;

struct usb_xfer_request;

/**
 * @ingroup usbcore
 *
 * @def usb_xfer_completed_t
 *
 * 非同期（割り込み駆動）USB転送が完了、または失敗したときに呼び出される
 * 関数の型.
 * これはIRQを無効にして呼び出される。一般に、このコールバックは受信した
 * データを何らかの方法で処理し（転送が入力で成功した場合）、その後、
 * struct usb_xfer_requestを解放、または再投入することが期待されている。
 */
typedef void (*usb_xfer_completed_t)(struct usb_xfer_request *req);

/**
 * @ingroup usbcore
 *
 * @struct usb_xfer_request
 *
 * 非同期（割り込み駆動）USB転送要求構造体.
 * コントロール、バルク、インターラプトのいずれかの転送要求である。
 * この構造体を取得するには usb_alloc_xfer_request() を呼び出すか、
 * 別の方法で手動でメモリを割り当てて usb_init_xfer_request() を呼び
 * 出す。以下のメンバーを設定した後、usb_submit_xfer_request() を使って
 * USBコアに送信する。
 * @ref usb_xfer_request::dev "dev",
 * @ref usb_xfer_request::endpoint_desc "endpoint_desc",
 * @ref usb_xfer_request::sendbuf "sendbuf" または
 * @ref usb_xfer_request::recvbuf "recvbuf",
 * @ref usb_xfer_request::size "size",
 * @ref usb_xfer_request::completion_cb_func "completion_cb_func"
 * 以下はオプション
 * @ref usb_xfer_request::private "private"
 * @ref usb_xfer_request::setup_data "setup_data" members.
 */
struct usb_xfer_request
{
    /**  [in] 通信するUSBデバイス */
    struct usb_device *dev;

    /**  [in] 通信するエンドポイントのディスクリプタへのポインタ。
     * このポインタを取得するには struct usb_device の endpoints 配列を
     * 検索する。デフォルトのコントロールエンドポイントを指定する場合は
     * @c NULL を使用する。
     */
    const struct usb_endpoint_descriptor *endpoint_desc;

    /**  [in] データバッファ: endpoint_number が INエンドポイント、
     * OUTエンドポイントのいずれを指定するかにより sendbufか
     * recvbufのいずれかが使用される。
     */
    union {
        /** 送信データのバッファ。sizeが0の場合は無視される。  */
        void *sendbuf;
        /** 受信データを書き込むバッファ。sizeが0の場合は無視される。 */
        void *recvbuf;
    };

    /** [in] sendbufまたはrecvbufのサイズ。INエンドポイントの場合、これは
     * 受信するデータの最大バイト数である。OUTエンドポイントの場合は
     * 送信するデータの正確なバイト数である。
     */
    uint size;

    /** [in] USBコントロール要求のためのデータを設定する。コントロール転送では
     * 必ず設定する必要がある。その他の場合は、無視される。注: コントロール
     * 転送では代わりに usb_control_msg() の仕様を検討されたい。
     */
    struct usb_control_setup_data setup_data;

    /** [in] このUSB転送が成功裏に完了した、または、失敗した場合に呼び出される
     * コールバック関数。
     */
    usb_xfer_completed_t completion_cb_func;

    /** [in] 完了コールバック用に保存されるUSBデバイスドライバのプライベートデータ。
     * このメンバの設定はオプションである。
     */
    void *private;

    /** [out] 転送のステータス: 転送に成功した場合は ::USB_STATUS_SUCCESS
     * 失敗した場合は別の ::usb_status_t エラーコード。
     * ::USB_STATUS_SUCCESS が設定されるのは、要求されたバイト数が正確に
     * エラーなく転送された場合、またはデバイスからホストへの (IN) 転送で
     * エラーなく完了したが要求より少ないバイト数で返された場合である。
     */
    usb_status_t status;

    /** [out] 転送された実際のデータサイズ。デバイスからホストへの (IN) 転送が
     * 成功した場合、これはテストされるべきである。何らかの理由でデバイスが
     * フルサイズを提供できなかった場合、要求されたバイト数よりも少ない
     * バイト数で転送を完了するが、これは正当な結果だからである。
     */
    uint actual_size;

    /*
     * プライベート変数（主にホストコントローラドライバ用でデバイス  *
     * ドライバからは触れたはならない）                              *
     * TODO: デザインを良くすればHCDが使用できる変数をカスタマイズ   *
     * することを可能できるかもしれない、おそらくusb_xfer_requestを  *
     * 別の構造体に埋め込めばいけるのではないか。                    *
     */
    /** [private] 現在のデータ位置 */
    void *cur_data_ptr;
    /** [private] 分割完了フラグ */
    uint8_t complete_split : 1;
    /** [private] 分割転送か */
    uint8_t short_attempt  : 1;
    /** [private] SOFが必要か */
    uint8_t need_sof       : 1;
    /** [private] コントロール転送のフェーズ */
    uint8_t control_phase  : 2;
    /** [private] 次に送信するデータのpid */
    uint8_t next_data_pid  : 2;
    /** [private] 転送サイズ */
    uint attempted_size;
    /** [private] 未転送のパケット数 */
    uint attempted_packets_remaining;
    /** [private] 未転送のバイト数 */
    uint attempted_bytes_remaining;
    /** [private] 分割リトライ数 */
    uint csplit_retries;
    /** [private] 遅延転送スレッドのtid */
    tid_typ deferer_thread_tid;
    /** [private] 遅延転送のためのセマフォ */
    semaphore deferer_thread_sema;
};

/**
 * @ingroup usbcore
 *
 * @struct usb_device_driver
 *
 * USBデバイスドライバ構造体.  これはドライバにより静的に宣言され、
 * その後 usb_register_device_driver() を使ってUSBコアドライバに
 * 登録する必要がある。この構造体で指定されているコールバック関数は
 * USBコアドライバから適切なタイミングで自動的に呼び出される。
 */
struct usb_device_driver
{
    /** デバイスの名前。情報メッセージ用のみ。 @c NULL でもよい。 */
    const char *name;

    /**
     * このUSBデバイスドライバをUSBデバイスにバインドするために呼び出される関数.
     * すべてのUSBデバイスドライバはこの関数を実装する必要があるため @c NULL を指定
     * することはできない。
     *
     * この関数の実装では、まずUSBデバイスがドライバによってサポートされているかを
     * チェックし、サポートされていない場合は ::USB_STATUS_DEVICE_UNSUPPORTED を
     * 返さなければならない。USBデバイスがサポートされているかどうかを判断するために
     * 実装は @ref usb_device::descriptor "デバイスディスクリプタ" の
     * @ref usb_device_descriptor::idVendor "製造者" と
     * @ref usb_device_descriptor::idProduct "製品" ID、または利用可能な
     * @ref usb_device::interfaces "インタフェース" と
     * @ref usb_device::endpoints "エンドポイント" を調べることができる。
     *
     * USBデバイスがドライバによってサポートされている場合、この関数は、デバイス固有
     * またはクラス固有のコントロールメッセージでデバイスを構成するなど、必要な
     * デバイス固有の設定を行い、デバイスをサポートするために必要なリソース（たとえば、
     * @ref ::usb_xfer_request "USB transfer requests" など) を割り当てなければならない。
     * 完全に成功した場合は、USB_STATUS_SUCCESS を返さなければならない。そうでない場合は、
     * デバイスに割り当てられたリソースをすべて解放し、 usb_status_t エラーコードを
     * 返さなければならない。必要であれば、デバイスドライバはUSBデバイス構造体の
     * @ref usb_device::driver_private "driver_private" メンバにドライバ固有のデータを
     * デバイス毎に格納することができる。
     *
     * この関数はドライバがあるUSBデバイスに正常にバインドされた後であっても
     * USBコアにより続けて @a 別のUSBデバイス を引数に呼び出される可能性がある。
     * デバイスが複数のUSBデバイスを同時に制御できない場合、この関数は
     * ::USB_STATUS_DEVICE_UNSUPPORTED を返さなければならない。ただし、この関数は
     * 他の @ref usb_device_driver::bind_device "bind_device" や
     * @ref usb_device_driver::unbind_device "unbind_device" の呼び出しと
     * @a 同時に 呼び出されることはない。
     *
     * この関数が呼ばれたとき、USBデバイスはリストの先頭にあるコンフィグレーションで
     * 構成されている。現在のところ、デバイスドライバが別のコンフィグレーションを選択
     * する方法はない。
     */
    usb_status_t (*bind_device)(struct usb_device *dev);

    /**
     * デバイスがUSBから取り外されたために、USBデバイスドライバの
     * バインドを解除するために呼び出される関数.  これは、
     * @ref usb_device_driver::bind_device "bind_device" が正常に
     * 復帰した後は、USBコアから任意の時点で呼び出される可能性がある。
     * この関数は @ref usb_device_driver::bind_device "bind_device" で
     * USBデバイスに割り当てられたリソースを解放する役割を担っている。
     * デバイスドライバはこの関数の実装を厳密には要求されておらず、
     * ここでは @c NULL を指定することができる。ただし、その場合、
     * @ref usb_device_driver::bind_device "bind_device" で割り当てられた
     * リソースはデバイスが切り離された場合にリークする。
     *
     * この関数が呼ばれた時には、対応するUSBデバイスへの保留中の転送は
     * ないことが保証されている。これを実現するために、この関数が呼ばれる前の
     * 一定期間、USBコアは usb_submit_xfer_request()、または、
     * usb_control_msg() でそのデバイスに転送を行おうとしても
     * ::USB_STATUS_DEVICE_DETACHED で失敗するようにしている。
     * usb_submit_xfer_request() の場合、完了コールバックは呼び出されない。
     * さらに、すべての保留中の転送で完了コールバックが呼び出され、
     * それらのステータスには ::USB_STATUS_DEVICE_DETACHED が設定される。
     * この結果、このアンバインドコールバックが呼ばれる際には、割り当てられた
     * すべての ::usb_xfer_request 構造体はドライバが所有することに
     * なる。したがって、これらの構造体はドライバがそれらの参照を追加で
     * 保持しない限り、直ちに解放することが可能である。
     *
     * この関数は、常にIRQが有効の状態で呼び出され、同じドライバにバインド
     * されている @a 別の デバイスのバインドを解除するために連続して複数回
     * 呼び出される場合がある。ただし、 @ref usb_device_driver::bind_device "bind_device"
     * または、 @ref usb_device_driver::unbind_device "unbind_device" の
     * 呼び出しと @a 同時に 呼び出されることはない。
     */
    void (*unbind_device)(struct usb_device *dev);
};

/** @ingroup usbcore
 * @def USB_DEVICE_MAX_INTERFACES
 * デバイスあたりの最大インタフェース数  */
#define USB_DEVICE_MAX_INTERFACES 8

/** @ingroup usbcore
 * @def USB_DEVICE_MAX_ENDPOINTS
 * インタフェースあたりの最大エンドポイント数  */
#define USB_DEVICE_MAX_ENDPOINTS  8

/**
 * @ingroup usbcore
 *
 * @struct usb_device
 *
 * USBデバイス構造体. これは、USBコアからデバイスドライバの
 * @ref usb_device_driver::bind_device "bind_device" コールバックと
 * @ref usb_device_driver::unbind_device "unbind_device" コールバックに
 * 提供される。（ハブドライバを除く）デバイスドライバはこの構造体の
 * 割り当てと解放に責任を持たない。
 */
struct usb_device
{
    /** この構造体が使用中の場合は TRUE; 新しいデバイスに割り当て可能な
     * 場合は FALSE。USBコアによりセットされる。 */
    bool inuse;

    /** このデバイスのアドレス。USBコアによりセットされる。  */
    uint address;

    /** このデバイスが接続されているハブの深さレベル（ルートハブは0、次のレベルのハブは1, ...）。
     * USBコアによりセットされる  */
    uint depth;

    /** このデバイスが接続されている親ハブ上のUSBポートの1始まりのインデックス。
     * または、これがルートハブの場合は0。USBコアによりセットされる  */
    uint port_number;

    /** このデバイスがハブに接続されたスピード。USBコアによりセットされる  */
    enum usb_speed speed;

    /** このUSBデバイスが接続されたハブ。または、これがルートハブの場合は @c NUL 。
     * USBコアによりセットされる  */
    struct usb_device *parent;

    /** このUSBデバイスの現在のコンフィグレーションのインデックス。USBコアによりセットされる  */
    uint8_t configuration;

    /** このデバイスのデバイスディスクリプタ。USBコアによりセットされる  */
    struct usb_device_descriptor descriptor;

    /** このデバイスの最初のコンフィグレーションの完全なコンフィグレーション
     * ディスクリプタへのポインタ。USBコアによりセットされる  */
    struct usb_configuration_descriptor *config_descriptor;

    /** このデバイスのすべてのインタフェースディスクリプタへのポインタ。
     * USBコアによりセットされる  */
    struct usb_interface_descriptor *interfaces[USB_DEVICE_MAX_INTERFACES];

    /** インタフェースにより用意されているこのデバイスのすべてのエンドポイント
     * ディスクリプタへのポインタ。USBコアによりセットされる  */
    struct usb_endpoint_descriptor *endpoints[
                            USB_DEVICE_MAX_INTERFACES][USB_DEVICE_MAX_ENDPOINTS];

    /** このデバイスのヌル終端された製品文字列（ASCIIエンコードで、可能であれば英語）。
     * デバイスが製品文字列を持たない、または読み取れなかった場合は空文字列となる。
     * USBコアによりセットされる  */
    char product[128];

    /** このデバイスのヌル終端された製造者文字列（ASCIIエンコードで、可能であれば英語）。
     * デバイスが製造者文字列を持たない、または読み取れなかった場合は空文字列となる。
     * USBコアによりセットされる  */
    char manufacturer[128];

    /** このUSBデバイスのドライバ用のプライベートデータ。USBデバイスドライバは自身の
     * @ref usb_device_driver::bind_device "bind_device" 関数でプライベートデータ構造体を
     * このフィールドに設定することができる。  */
    void *driver_private;

    /** もしあれば、このデバイスにバインドされたドライバ。 USBコアによりセットされる */
    const struct usb_device_driver *driver;

    /** このデバイスで失敗状態で完了したUSB転送の数  */
    ulong error_count;

    /** このデバイスで起きた最新のエラー  */
    usb_status_t last_error;

    /** デバイスの接続状態; USBコアのみ使用  */
    enum {
        USB_DEVICE_ATTACHED,
        USB_DEVICE_DETACHMENT_PENDING,
    } state;

    /** 保留中の転送数: USBコアのみ使用  */
    uint xfer_pending_count;

    /** 転送完了を待機中のタスク: USBコアのみ使用  */
    tid_typ quiescent_state_waiter;
};


/* 以下の関数はUSBデバイスドライバが使用することを意図している  */

usb_status_t
usb_register_device_driver(const struct usb_device_driver *drv);

struct usb_xfer_request *
usb_alloc_xfer_request(uint bufsize);

void
usb_init_xfer_request(struct usb_xfer_request *req);

void
usb_free_xfer_request(struct usb_xfer_request *req);

usb_status_t
usb_submit_xfer_request(struct usb_xfer_request *req);


usb_status_t
usb_control_msg(struct usb_device *dev,
                const struct usb_endpoint_descriptor *desc,
                uint8_t bRequest, uint8_t bmRequestType,
                uint16_t wValue, uint16_t wIndex, void *data, uint16_t wLength);

usb_status_t
usb_get_descriptor(struct usb_device *dev, uint8_t bRequest,
		   uint8_t bmRequestType, uint16_t wValue, uint16_t wIndex,
		   void *buf, uint16_t buflen);

/* 以下の関数は非組み込みのUSBコードでのみ利用可能  */

#if !USB_EMBEDDED
usb_status_t
usb_get_string_descriptor(struct usb_device *dev, uint8_t index, uint16_t lang_id,
                          struct usb_string_descriptor *buf, uint16_t buflen);

usb_status_t
usb_get_ascii_string(struct usb_device *dev, uint32_t iString,
                     char *strbuf, uint32_t strbufsize);

const char *
usb_device_description(const struct usb_device *dev);

const char *
usb_class_code_to_string(enum usb_class_code class_code);

const char *
usb_transfer_type_to_string(enum usb_transfer_type type);

const char *
usb_direction_to_string(enum usb_direction dir);

const char *
usb_speed_to_string(enum usb_speed speed);

#endif /* !USB_EMBEDDED */

/* 以下の関数はUSBハブデバイスドライバだけが使用することを意図している。  */

struct usb_device *
usb_alloc_device(struct usb_device *parent);

usb_status_t
usb_attach_device(struct usb_device *dev);

void
usb_free_device(struct usb_device *dev);

/* 以下の関数はUSBホストコントローラドライバだけが使用することを意図している。  */

/**
 * @ingroup usbcore
 *
 * 指定されたUSB転送要求がコントロール要求か. USBホストコントローラドライバだけが使用。
 * @param req USB転送要求へのポインタ
 * @return コントロール要求の場合は TRUE; そうでなければ FALSE
 */
static inline bool
usb_is_control_request(const struct usb_xfer_request *req)
{
    return req->endpoint_desc == NULL ||
           (req->endpoint_desc->bmAttributes & 0x3) == USB_TRANSFER_TYPE_CONTROL;
}

/**
 * @ingroup usbcore
 * 指定されたUSB転送要求がインターラプト要求か. USBホストコントローラドライバだけが使用。
 * @param req USB転送要求へのポインタ
 * @return インターラプト転送の場合は TRUE; そうでなければ FALSE.
 */
static inline bool
usb_is_interrupt_request(const struct usb_xfer_request *req)
{
    return req->endpoint_desc != NULL &&
           (req->endpoint_desc->bmAttributes & 0x3) == USB_TRANSFER_TYPE_INTERRUPT;
}

void
usb_complete_xfer(struct usb_xfer_request *req);

void usb_lock_bus(void);

void usb_unlock_bus(void);

/** 以下の関数はUSBホストコントローラドライバとハブドライバが使用することを
 * 主に意図している。その他のドライバにはおそらく必要がない。
 * */

/**
 * @ingroup usbcore
 * USBデバイスがルートハブか. USBホストコントローラドライバとハブドライバが使用。
 * @param dev USBデバイス
 * @return ルートハブの場合は TRUE; そうでなければ FALSE
 */
static inline bool is_root_hub(const struct usb_device *dev)
{
    return dev->parent == NULL;
}

/**
 * @ingroup usbcore
 * USBデバイスがハブか. USBホストコントローラドライバとハブドライバが使用。
 * @param dev USBデバイス
 * @return ハブの場合は TRUE; そうでなければ FALSE
 */
static inline bool is_hub(const struct usb_device *dev)
{
    return dev->descriptor.bDeviceClass == USB_CLASS_CODE_HUB;
}


#endif /* _USB_CORE_DRIVER_H_ */
