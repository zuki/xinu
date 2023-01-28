/**
 * @file usb_core_driver.h
 * @ingroup usbcore
 *
 * コアUSBドライバへのインタフェース。これはUSBデバイスドライバと
 * ある程度はホストコントローラドライバにより仕様されることを意図
 * している。
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
 * 非同期（割り込み駆動）USB転送が完了、または失敗したときに呼び出される
 * 関数の型。
 * これはIRQを無効にして呼び出される。一般に、このコールバックは受信した
 * データを何らかの方法で処理し（転送が入力で成功した場合）、その後、
 * struct usb_xfer_requestを解放、または再投入することが期待されている。
 */
typedef void (*usb_xfer_completed_t)(struct usb_xfer_request *req);

/**
 * @ingroup usbcore
 *
 * 非同期（割り込み駆動）USB 転送要求の仕様。
 * コントロール、バルク、インターラプトのいずれかの転送要求である。
 * これらの構造体を取得するには usb_alloc_xfer_request() を呼び出すか、
 * 別の方法で手動でメモリを割り当てて usb_init_xfer_request() を呼び
 * 出し、以下のメンバーを設定した後、usb_submit_xfer_request() を使って
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

    /*********************
     * 入呂っく変数      *
     *********************/

    /** 通信相手のUSBデバイス  */
    struct usb_device *dev;

    /** 通信相手となるエンドポイントのディスクリプタへのポインタ。
     * このポインタを取得するには struct usb_device の endpoints 配列を
     * 検索する。デフォルトのコントロールエンドポイントを指定する場合は
     * @c NULL を使用する。
     */
    const struct usb_endpoint_descriptor *endpoint_desc;

    /** データバッファ: endpoint_number が INエンドポイント、
     * OUTエンドポイントのいずれを指定するかにより sendbufか
     * recvbufのいずれかが使用される。
     */
    union {
        /** 送信データのバッファ。sizeが0の場合は無視される。  */
        void *sendbuf;
        /** 受信データを書き込むバッファ。sizeが0の場合は無視される。 */
        void *recvbuf;
    };

    /** sendbufまたはrecvbufのサイズ。INエンドポイントの場合、これは
     * 受信するデータの最大バイト数である。OUTエンドポイントの場合は
     * 送信するデータの正確なバイト数である。
     */
    uint size;

    /** USBコントロール要求のためのデータを設定する。コントロール転送では
     * 必ず設定する必要がある。その他の場合は、無視される。注: コントロール
     * 転送では代わりに usb_control_msg() の仕様を検討されたい。
     */
    struct usb_control_setup_data setup_data;

    /** このUSB転送が成功裏に完了した、または、失敗した場合に呼び出される
     * コールバック関数。
     */
    usb_xfer_completed_t completion_cb_func;

    /** 完了コールバック用に保存されるUSBデバイスドライバプライベートデータ。
     * このメンバの設定はオプションである。
     */
    void *private;

    /*********************
     * 出力変数          *
     *********************/

    /** 転送のステータス: 転送に成功した場合は ::USB_STATUS_SUCCESS、
     * 失敗した場合は別の ::usb_status_t エラーコード。
     * ::USB_STATUS_SUCCESS が設定されるのは、正確に要求されたバイト数が
     * エラーなく転送された場合、またはデバイスからホストへの (IN) 転送で
     * エラーなく完了したが要求より少ないバイト数で返された場合である。
     */
    usb_status_t status;

    /** 転送されたデータの実際のサイズ。デバイスからホストへの (IN) 転送が
     * 成功した場合、これはテストされるべきである。何らかの理由でデバイスが
     * フルサイズを提供できなかった場合、要求されたバイト数よりも少ない
     * バイト数で転送を完了するが、これは正当な結果だからである。
     */
    uint actual_size;

    /*****************************************************************
     * プライベート変数（主にホストコントローラドライバ用でデバイス  *
     * ドライバからは触れない）。                                    *
     * TODO: デザインを良くすればHCDが使用できる変数をカスタマイズ   *
     * することを可能にするかもしれない、おそらくusb_xfer_requestを  *
     * 別の構造体に埋め込めば。                                      *
     *****************************************************************/
    void *cur_data_ptr;
    uint8_t complete_split : 1;
    uint8_t short_attempt  : 1;
    uint8_t need_sof       : 1;
    uint8_t control_phase  : 2;
    uint8_t next_data_pid  : 2;
    uint attempted_size;
    uint attempted_packets_remaining;
    uint attempted_bytes_remaining;
    uint csplit_retries;
    tid_typ deferer_thread_tid;
    semaphore deferer_thread_sema;
};

/**
 * @ingroup usbcore
 *
 * Information about a USB device driver.  This should be declared staticly by
 * the driver, then registered with the USB core driver using
 * usb_register_device_driver().  The callback functions specified in the
 * structure will then be called automatically by the USB core driver at
 * appropriate times.
 */
struct usb_device_driver
{
    /** デバイスの名前。情報メッセージ用のみ。 @c NULL でもよい。 */
    const char *name;

    /**
     * このUSBデバイスドライバをUSBデバイスにバインドするために呼び出される関数である。
     * すべてのUSBデバイスドライバはこの関数を実装する必要があるため、@c NULL を指定
     * することはできない。
     *
     * この関数の実装では、まずUSBデバイスがドライバによってサポートされているかを
     * チェックし、サポートされていない場合は ::USB_STATUS_DEVICE_UNSUPPORTED を
     * 返さなければならない。USBデバイスがサポートされているかどうかを判断するために
     * 実装は @ref usb_device::descriptor ”device descriptor" の
     * @ref usb_device_descriptor::idVendor "vendor" と
     * @ref usb_device_descriptor::idProduct "product" ID、または利用可能な
     * @ref usb_device::interfaces "interfaces" と
     * @ref usb_device::endpoints "endpoints" を調べることができる。
     *
     * USBデバイスがドライバによってサポートされている場合、この関数は、デバイス固有
     * またはクラス固有のコントロールメッセージでデバイスを構成するなど、必要な
     * デバイス固有の設定を行い、デバイスをサポートするために必要なリソース（たとえば、
     * @ref ::usb_xfer_request "USB transfer requests" など) を割り当てなければならない。
     * 完全に成功した場合は、USB_STATUS_SUCCESS を返さなければならない。そうでない場合は、
     * デバイスに割り当てられたリソースをすべて解放し、別の usb_status_t エラーコードを
     * 返さなければならない。必要であれば、デバイスドライバはUSBデバイス構造体の
     * @ref usb_device::driver_private "driver_private" メンバにドライバ特有のデータを
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
     * この関数が呼ばれたとき、USBデバイスは最初にリストされた構成ですでに
     * 構成されている。現在のところ、デバイスドライバが別の構成を選択する方法は
     * ない。
     */
    usb_status_t (*bind_device)(struct usb_device *dev);

    /**
     * デバイスがUSBから取り外されたために、USBデバイスドライバの
     * バインドを解除するために呼び出される関数。これは、
     * @ref usb_device_driver::bind_device "bind_device" が正常に
     * 復帰した後は、USBコアから任意の時点で呼び出される可能性がある。
     * この関数は @ref usb_device_driver::bind_device "bind_device " で
     * USBデバイスに割り当てられたリソースを解放する役割を担っている。
     * デバイスドライバはこの関数の実装を厳密には要求されておらず、
     * ここでは @c NULL を指定することができる。ただし、その場合、
     * @ref usb_device_driver::bind_device "bind_device" で割り当てられた
     * リソースはデバイスが切り離された場合にリークする。
     *
     * この関数が呼ばれた時には、対応するUSBデバイスへの保留中の転送は
     * ないことが保証される。これを実現するために、この関数が呼ばれる前の
     * 一定期間、USBコアは usb_submit_xfer_request()、または、
     * usb_control_msg() でそのデバイスに転送を行おうとしても
     * ::USB_STATUS_DEVICE_DETACHED で失敗するようにする。
     * usb_submit_xfer_request() の場合、完了コールバックは呼び出されない。
     * さらに、すべての保留中の転送で完了コールバックが呼び出され、
     * それらのステータスが ::USB_STATUS_DEVICE_DETACHED に設定される。
     * この結果、このアンバインドコールバックが呼ばれると、割り当てられた
     * すべての ::usb_xfer_request 構造体はドライバにより所有されることに
     * なり、ドライバがそれらの参照を追加で保持しない限り、そのような構造体は
     * すぐに解放することが可能である。
     *
     * この関数は、常にIRQが有効の状態で呼び出され、同じドライバにバインド
     * されている @a 別の デバイスのバインドを解除するために連続して複数回
     * 呼び出される場合がある。ただし、 @ref usb_device_driver::bind_device "bind_device"
     * または、 @ref usb_device_driver::unbind_device "unbind_device" の
     * 呼び出しと @a 同時に 呼び出されることはない。
     */
    void (*unbind_device)(struct usb_device *dev);
};

/** デバイスあたりの最大インタフェース数  */
#define USB_DEVICE_MAX_INTERFACES 8

/** インタフェース足りの最大エンドポイント数  */
#define USB_DEVICE_MAX_ENDPOINTS  8

/**
 * @ingroup usbcore
 *
 * USBデバイスに関する情報。これは、USBコアからデバイスドライバの
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

    /** 今デバイスのアドレス。USBコアによりセットされる。  */
    uint address;

    /** このデバイスの深さ（ルートハブは0、次のレベルのハブは1, ...）。
     * USBコアによりセットされる  */
    uint depth;

    /** このデバイスが接続された親ハブ上のUSBポートの1始まりのインデックス。
     * または、これがルートハブの場合は0。USBコアによりセットされる  */
    uint port_number;

    /** このデバイスがハブに接続されたスピード。USBコアによりセットされる  */
    enum usb_speed speed;

    /** このUSBデバイスが接続されたハブ。または、これがルートハブの場合は @c NUL 。
     * USBコアによりセットされる  */
    struct usb_device *parent;

    /** このUSBデバイスの現在の構成インデックス。USBコアによりセットされる  */
    uint8_t configuration;

    /** このデバイスのデバイスディスクリプタ。USBコアによりセットされる  */
    struct usb_device_descriptor descriptor;

    /** このデバイスの最初のコンフィグレーションの完全なコンフィグレーション
     * ディスクリプタへのポインタ。USBコアによりセットされる  */
    struct usb_configuration_descriptor *config_descriptor;

    /** このデバイスのすべてのインタフェースディスクリプタへのポインタ。
     * USBコアによりセットされる  */
    struct usb_interface_descriptor *interfaces[USB_DEVICE_MAX_INTERFACES];

    /** インタフェースにより用意されたこのデバイスのすべてのエンドポイント
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

    /** このUSBデバイスのドライバ用のプライベートデータ。USBデバイスドライバは
     * @ref usb_device_driver::bind_device "bind_device" 関数のここにプライベート
     * データ構造体を置くことができる。  */
    void *driver_private;

    /** もしあれば、このデバイスにバインドされたドライバ。 USBコアによりセットされる */
    const struct usb_device_driver *driver;

    /** このデバイスで失敗状態で完了したUSB転送の数  */
    ulong error_count;

    /** このデバイスで行った最新のエラー  */
    usb_status_t last_error;

    /** USBコアのみ使用  */
    enum {
        USB_DEVICE_ATTACHED,
        USB_DEVICE_DETACHMENT_PENDING,
    } state;
    /** USBコアのみ使用  */
    uint xfer_pending_count;
    /** USBコアのみ使用  */
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
 * 指定されたUSB転送要求がコントロール要求の場合は TRUE; そうでなければ FALSE
 */
static inline bool
usb_is_control_request(const struct usb_xfer_request *req)
{
    return req->endpoint_desc == NULL ||
           (req->endpoint_desc->bmAttributes & 0x3) == USB_TRANSFER_TYPE_CONTROL;
}

/**
 * R指定されたUSB転送要求がインターラプト要求の場合は TRUE; そうでなければ FALSE
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

/** 以下の関数はUSBホストコントローラドライバとハブドライバシユすることを
 * 主に意図している。その他のドライバにはおそらく必要がない。
 * */

/**
 * USBデバイスがルートハブの場合は TRUE; そうでなければ FALSE
 */
static inline bool is_root_hub(const struct usb_device *dev)
{
    return dev->parent == NULL;
}

/**
 * USBデバイスがハブの場合は TRUE; そうでなければ FALSE
 */
static inline bool is_hub(const struct usb_device *dev)
{
    return dev->descriptor.bDeviceClass == USB_CLASS_CODE_HUB;
}


#endif /* _USB_CORE_DRIVER_H_ */
