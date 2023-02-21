/**
 * @file usb_dwc_hcd.c
 *
 * このファイルにはSynopsys DesignWare Hi-Speed USB 2.0 On-The-Go
 * Controller用のUSBホストコントローラデバイスが含まれている。
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

/**
 *
 * これはRaspberry Piに搭載されているBCM2835 SoCで使用されているUSBホスト
 * コントローラである。
 *
 * このハードウェアに関する公的に利用できる正式なドキュメントが存在しない
 * こと、また、EHCIのような標準的なインタフェースではなく独自のカスタム
 * ホストコントローラインタフェースを使用していることに注意されたい。
 * そのため、このドライバは、必要なハードウェアの詳細を得るために、ベンダーが
 * 提供した非常に複雑で理解しにくいLinuxドライバなどのいくつかの情報源を
 * 使用して、ベストエフォートベースで書かれている。
 *
 * このファイルでは usb_hcdi.h で定義されているホストコントローラドライバ
 * インタフェースを実装している。最も重要なのは、ホストコントローラの電源
 * 投入と起動を行う関数 (hcd_start()) と、USB上でメッセージを送受信する
 * 関数 (hcd_submit_xfer_request()) を実装していることである。
 *
 * DWCはメモリマップドレジスタの読み書きにより制御される。最も重要な
 * レジスタはホストチャネルレジスタである。このハードウェアにおいて
 * 「ホストチャネル」または単に「チャネル」とは、ソフトウェアがUSB上で
 * トランザクションを行えるようにするために読み書きができるレジスタの
 * セットである。ホストチャネルの数は決まっており、Raspberry Piでは8つ
 * である。ソフトウェアから見ると、異なるホストチャネルを使用した
 * トランザクションを同時に実行することが可能である。
 *
 * ホストチャネルレジスタの中には割り込みをあつかつものがあります。
 * このドライバはこれらを多用し、すべてのUSB転送を割り込み駆動で行っている。
 * しかし、このハードウェアとUSB2.0自体の設計上の欠陥により「インターラプト」
 * 転送と「アイソクロナス」転送では個々の転送が割り込み駆動であったとしても
 * 依然として、新しいデータの確認にはソフトウェアポーリングを使用する必要が
 * ある。これはたとえば、USBマウスのポーリングレートを100回/秒と指定した場合、
 * 残念ながらソフトウェアでは100回/秒のポーリングが行われることを意味する。
 * この特殊なハードウェアで割り込みがどのように制御されるかについての詳細は、
 * dwc_setup_interrupts() のコメントを参照されたい。
 *
 * もう1つ重要な概念は「パケット」、「トランザクション」、「転送」という
 * アイデアである。コントロールメッセージ、またはバルクリクエストのような
 * 1つのUSB転送はそれがエンドポイントの最大パケットサイズを超える場合、
 * 複数のパケットに分割する必要がある場合がある。残念ながらこのハードウェアは
 * それをやってくれないのでこのコードで明示的に対処する必要がある。ただし、
 * 少なくともこのソフトウェアの観点からは、「トランザクション」と「パケット」は
 * 本質的に同じものである。
 *
 * このハードウェアの名前にある "On-The-Go" は、このハードウェアがUSB
 * On-The-Goプロトコルをサポートしていることを意味しており、このハード
 * ウェアはホストとしてもデバイスとしても動作することが可能である。ただし、
 * ここではホストとして動作だけに関心があるのでドライバは簡潔にすることが
 * できる。
 *
 * USBコアソフトウェアを簡潔にするために（USB 2.0使用で推奨されており、
 * Linuxなどの他の実装で使用されている）有用な設計技法は、たとえルートハブが
 * ホストコントローラと統合されており、ハードウェアレベルでは標準ハブとは
 * 見えないとしても、HCDにルートハブを標準USBハブとして提示させるという
 * ものです。これはDWCにも当てはまり、この設計を実装しています。したがって、
 * このファイルのコードの一部はルートハブに送信するフェイクリクエストを
 * 扱っている。
 */

#include <compiler.h>
#include <interrupt.h>
#include <mailbox.h>
#include <string.h>
#include <thread.h>
#include <usb_core_driver.h>
#include <system/arch/arm/usb_dwc_regs.h>
#include <usb_hcdi.h>
#include <usb_hub_defs.h>
#include <usb_std_defs.h>
#include "bcm2835.h"

/** ワードサイズの次の倍数まで数値を丸めあげる  */
#define WORD_ALIGN(n) (((n) + sizeof(ulong) - 1) & ~(sizeof(ulong) - 1))

/** ポインタがワード境界にあるか判定する  */
#define IS_WORD_ALIGNED(ptr) ((ulong)(ptr) % sizeof(ulong) == 0)

/** Synopsys DesignWare Hi-Speed USB 2.0 OTG Controlleのメモリ
 * マップドレジスタへのポインタ.  */
static volatile struct dwc_regs * const regs = (void*)DWC_REGS_BASE;

/**
 * USBエンドポイントの最大パケットサイズ.  1024 はUSB 2.0で許された最大数。
 * ほとんどのエンドポイントはこれより小さな最大パケットサイズを与えることになる。
 */
#define USB_MAX_PACKET_SIZE 1024


/**
 * USB転送リクエストスケジューラスレッドのスタックサイズ
 * （非常に小さくすることが可能）.
 */
#define XFER_SCHEDULER_THREAD_STACK_SIZE 4096

/**
 * USB転送リクエストスケジューラスレッドの優先度（USB転送ができるだけ
 * 早く開始できるように非常に高くするべきである）.
 */
#define XFER_SCHEDULER_THREAD_PRIORITY 60

/** USB転送リクエストスケジューラスレッドの名前 */
#define XFER_SCHEDULER_THREAD_NAME "USB scheduler"

/** USB遅延転送スレッドのスタックサイズ（非常に小さくすることが可能） */
#define DEFER_XFER_THREAD_STACK_SIZE 4096

/**
 * USB遅延転送スレッドの優先度（これらのスレッドは帯域幅が保証されている
 * 割り込みエンドポイントで必要なソフトウェアポーリングに使用される
 * ため、非常に高くするべきである）。
 */
#define DEFER_XFER_THREAD_PRIORITY 100

/**
 * USB遅延転送スレッド名前.  注: ヌル終端を含んでNMLEN以下で
 * あること、そうでなければ切り詰められることになる。
 */
#define DEFER_XFER_THREAD_NAME "USB defer xfer"

/** TODO: 適切であればこれを削除する */
#define START_SPLIT_INTR_TRANSFERS_ON_SOF 1

/** DWCハードウェアが認識するUSBパケットID定数  */
enum dwc_usb_pid {
    DWC_USB_PID_DATA0 = 0,
    DWC_USB_PID_DATA1 = 2,
    DWC_USB_PID_DATA2 = 1,
    DWC_USB_PID_SETUP = 3,
};

/** USB転送リクエストスケジューラスレッドのスレッドID  */
static tid_typ dwc_xfer_scheduler_tid;

/** チャネルステータスを示すビットマップ: 1: 空き、0: 使用中  */
static uint chfree;

#if START_SPLIT_INTR_TRANSFERS_ON_SOF
/** SOF (start-of-frame) を待機しているチャネルビットマップ */
static uint sofwait;
#endif

/** chfreeビットマップ内の空きチャネルを追跡するセマフォ  */
static semaphore chfree_sema;

/**
 * 各ハードウェアチャネルで現在完了しているUSB転送リクエスト
 * （もしあれば）へのポインタを保持する配列.
 */
static struct usb_xfer_request *channel_pending_xfers[DWC_NUM_CHANNELS];

/** DMA用のアライメントされたバッファ. */
static uint8_t aligned_bufs[DWC_NUM_CHANNELS][WORD_ALIGN(USB_MAX_PACKET_SIZE)]
                    __aligned(4);

/** 非0のワード内で最初に1がセットされているビットの位置を探す.  */
static inline ulong first_set_bit(ulong word)
{
    return 31 - __builtin_clz(word);
}

/**
 * 未使用のDWC USBホストチャネルを見つけて予約する.
 * これはブロッキングであり、チャネルが利用可能になるまで待機する。
 *
 * @return
 *      空きチャネルのインデックス.
 */
static uint
dwc_get_free_channel(void)
{
    uint chan;
    irqmask im;

    im = disable();
    wait(chfree_sema);
    chan = first_set_bit(chfree);
    chfree ^= (1 << chan);
    restore(im);
    return chan;
}

/**
 * チャネルに秋マークを付ける.  秋チャネルを待っているスレッドに通知する。
 *
 * @param chan
 *      開放するDWC USBホストチャネルのインデックス.
 */
static void
dwc_release_channel(uint chan)
{
    irqmask im;

    im = disable();
    chfree |= (1 << chan);
    signal(chfree_sema);
    restore(im);
}

/**
 * DWCハードウェアに電源を投入する.
 */
static usb_status_t
dwc_power_on(void)
{
    int retval;

    usb_info("Powering on Synopsys DesignWare Hi-Speed "
             "USB 2.0 On-The-Go Controller\n");
    retval = board_setpower(POWER_USB, TRUE);
    return (retval == OK) ? USB_STATUS_SUCCESS : USB_STATUS_HARDWARE_ERROR;
}

static void
dwc_power_off(void)
{
    usb_info("Powering off Synopsys DesignWare Hi-Speed "
             "USB 2.0 On-The-Go Controller\n");
    board_setpower(POWER_USB, FALSE);
}

/**
 * DWCハードウェアのソフトウェアリセットを実行する.  注: DWC は最初の電源
 * 投入後はリセット状態にあるだ。したがって、これは厳密にはDWCがすでに電源
 * 投入済みの状態で hcd_start() に入った場合にのみ必要である（たとえば、
 * ソフトウェアから kexec() で直接新しいカーネルを起動する場合など)。
 */
static void
dwc_soft_reset(void)
{
    usb_debug("Resetting USB controller\n");

    /* ソフトリセットフラグをセットして、クリアされるまで待つ  */
    regs->core_reset = DWC_SOFT_RESET;
    while (regs->core_reset & DWC_SOFT_RESET)
    {
    }
}

/**
 * DWC OTG USBホストコントローラをDMAモードに設定する.  これによりホスト
 * コントローラはUSB転送を行う際にインメモリッファに直接アクセスする
 * ことが可能になる。DMAでアクセスするバッファはすべて4バイトアライ
 * メントである必要があることに注意された。さらに、L1データキャッシュが
 * 有効な場合は、キャッシュはARMプロセッサに内蔵されているため、キャッシュ
 * コヒーレンシを維持するために明示的にフラッシュする必要がある。
 * （XinuはL1データキャッシュを有効にしないので、現在、このドライバは
 * これを行っていない。)
 */
static void
dwc_setup_dma_mode(void)
{
    const uint32_t rx_words = 1024;  /* Rx FIFOの4バイトワード単位のサイズ */
    const uint32_t tx_words = 1024;  /* 非周期的Tx FIFO in 4-bytの4バイトワード単位のサイズ */
    const uint32_t ptx_words = 1024; /* 周期的Tx FIFO in 4-bytの4バイトワード単位のサイズ */

    /* まず、ホストコントローラのFIFOサイズを設定する。これはデフォルト値
     * （少なくともBroadcomのSynopsys USBブロックのインスタンス）では正しく
     * 動作しないため必須である。ソフトウェアがこれを行わないと、データの
     * 受信に失敗し、メモリ破壊が商事sて、事実上デバッグができない事態が
     * 発生する。これはこのドライバでDMAを使用し、それ以外の方法ではホスト
     * コントローラのFIFOと相互作用しない場合でも当てはまる。 */
    usb_debug("%u words of RAM available for dynamic FIFOs\n", regs->hwcfg3 >> 16);
    usb_debug("original FIFO sizes: rx 0x%08x,  tx 0x%08x, ptx 0x%08x\n",
              regs->rx_fifo_size, regs->nonperiodic_tx_fifo_size,
              regs->host_periodic_tx_fifo_size);
    regs->rx_fifo_size = rx_words;
    regs->nonperiodic_tx_fifo_size = (tx_words << 16) | rx_words;
    regs->host_periodic_tx_fifo_size = (ptx_words << 16) | (rx_words + tx_words);

    /* 適切なフラグを設定することで実際にDMAを有効にする。同時に Synopsys USB
     * ブロックのBroadcomインスタンスでのみ利用可能な追加フラグも設定する
     * （実際に必要であるかどうかは不明）。  */
    regs->ahb_configuration |= DWC_AHB_DMA_ENABLE | BCM_DWC_AHB_AXI_WAIT;
}

/**
 * ホストポート制御およびステータスレジスタを変更を意図して読み込む.
 * このレジスタのビット設計には一貫性がないため、意図しない1の書き戻しで
 * クリアされないようにライトクリアビットをゼロにする必要がある。
 */
static union dwc_host_port_ctrlstatus
dwc_get_host_port_ctrlstatus(void)
{
    union dwc_host_port_ctrlstatus hw_status = regs->host_port_ctrlstatus;

    hw_status.enabled = 0;
    hw_status.connected_changed = 0;
    hw_status.enabled_changed = 0;
    hw_status.overcurrent_changed = 0;
    return hw_status;
}

/**
 * DWCホストポート（ルートハブに論理的に接続されたUSBポート）に電源を投入する
 */
static void
dwc_power_on_host_port(void)
{
    union dwc_host_port_ctrlstatus hw_status;

    usb_debug("Powering on host port.\n");
    hw_status = dwc_get_host_port_ctrlstatus();
    hw_status.powered = 1;
    regs->host_port_ctrlstatus = hw_status;
}

/**
 * DWCホストポートをリセットする.
 */
static void
dwc_reset_host_port(void)
{
    union dwc_host_port_ctrlstatus hw_status;

    usb_debug("Resetting host port\n");

    /* ポートのリセットフラグをセットし、その後、一定時間待ってクリアする  */
    hw_status = dwc_get_host_port_ctrlstatus();
    hw_status.reset = 1;
    regs->host_port_ctrlstatus = hw_status;
    sleep(60);  /* (sleep for 60 milliseconds).  */
    hw_status.reset = 0;
    regs->host_port_ctrlstatus = hw_status;
}

/** フェイクルートハブ用のハードコードされたデバイスディスクリプタ  */
static const struct usb_device_descriptor root_hub_device_descriptor = {
    .bLength = sizeof(struct usb_device_descriptor),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_DEVICE,
    .bcdUSB = 0x200, /* USB version 2.0 (binary-coded decimal) */
    .bDeviceClass = USB_CLASS_CODE_HUB,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = 0,
    .idProduct = 0,
    .bcdDevice = 0,
    .iManufacturer = 0,
    .iProduct = 1,
    .iSerialNumber = 0,
    .bNumConfigurations = 1,
};

/** フェイクルート用のハードコードされたコンフィグレーション
 * ディスクリプタと関連するインタフェースディスクリプタと
 * エンドポイントディスクリプタ  */
static const struct {
    struct usb_configuration_descriptor configuration;
    struct usb_interface_descriptor interface;
    struct usb_endpoint_descriptor endpoint;
} __packed root_hub_configuration = {
    .configuration = {
        .bLength = sizeof(struct usb_configuration_descriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_CONFIGURATION,
        .wTotalLength = sizeof(root_hub_configuration),
        .bNumInterfaces = 1,
        .bConfigurationValue = 1,
        .iConfiguration = 0,
        .bmAttributes = USB_CONFIGURATION_ATTRIBUTE_RESERVED_HIGH |
                        USB_CONFIGURATION_ATTRIBUTE_SELF_POWERED,
        .bMaxPower = 0,
    },
    .interface = {
        .bLength = sizeof(struct usb_interface_descriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 1,
        .bInterfaceClass = USB_CLASS_CODE_HUB,
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface = 0,
    },
    .endpoint = {
        .bLength = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = 1 | (USB_DIRECTION_IN << 7),
        .bmAttributes = USB_TRANSFER_TYPE_INTERRUPT,
        .wMaxPacketSize = 64,
        .bInterval = 0xff,
    },
};

/** フェイクルート用のハードコードされた言語IDリスト  */
static const struct usb_string_descriptor root_hub_string_0 = {
    /* bLength is the base size plus the length of the bString */
    .bLength = sizeof(struct usb_string_descriptor) +
               1 * sizeof(root_hub_string_0.bString[0]),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {USB_LANGID_US_ENGLISH},
};

/** フェイクルート用のハードコードされた製品文字列  */
static const struct usb_string_descriptor root_hub_string_1 = {
    /* bLength is the base size plus the length of the bString */
    .bLength = sizeof(struct usb_string_descriptor) +
               16 * sizeof(root_hub_string_1.bString[0]),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,

    /* This is a UTF-16LE string, hence the array of individual characters
     * rather than a string literal.  */
    .bString = {'U', 'S', 'B', ' ',
                '2', '.', '0', ' ',
                'R', 'o', 'o', 't', ' ',
                'H', 'u', 'b'},
};

/** フェイクルート用のハードコードされた文字列テーブル.  */
static const struct usb_string_descriptor * const root_hub_strings[] = {
    &root_hub_string_0,
    &root_hub_string_1,
};

/** フェイクルート用のハードコードされたハブディスクリプタ  */
static const struct usb_hub_descriptor root_hub_hub_descriptor = {
    /* bDescLength is the base size plus the length of the varData */
    .bDescLength = sizeof(struct usb_hub_descriptor) +
                   2 * sizeof(root_hub_hub_descriptor.varData[0]),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_HUB,
    .bNbrPorts = 1,
    .wHubCharacteristics = 0,
    .bPwrOn2PwrGood = 0,
    .bHubContrCurrent = 0,
    .varData = { 0x00 /* DeviceRemovable */,
                 0xff, /* PortPwrCtrlMask */ },
};

/** フェイクルート用のハードコードされたハブステータス */
static const struct usb_device_status root_hub_device_status = {
    .wStatus = USB_DEVICE_STATUS_SELF_POWERED,
};

/**
 * ルートハブのステータス変化エンドポイントへの（もしあれば）
 * 保留中のインターラプト転送
 */
static struct usb_xfer_request *root_hub_status_change_request = NULL;

/**
 * ホストポートのステータス保存。これはホストポートのステータス変化により
 * ホストコントローラが割り込みを発行したときに変更される。直接ハードウェア
 * レジスタを使用せずに別の変数でこのステータスを追跡する必要があるのは、
 * 割り込みをクリアするためにハードウェアレジスタの変更をクリアする必要が
 * あるからである。
 */
static struct usb_port_status host_port_status;

/**
 * host_port_statusが更新された時に呼び出され、ルートハブに送信された
 * ステータス変化インターラプト転送を処理することができる。
 */
static void
dwc_host_port_status_changed(void)
{
    struct usb_xfer_request *req = root_hub_status_change_request;
    if (req != NULL)
    {
        root_hub_status_change_request = NULL;
        usb_debug("Host port status changed; "
                  "responding to status changed transfer on root hub\n");
        *(uint8_t*)req->recvbuf = 0x2; /* 0x2 means Port 1 status changed (bit 0 is
                                     used for the hub itself) */
        req->actual_size = 1;
        req->status = USB_STATUS_SUCCESS;
        usb_complete_xfer(req);
    }
}


/**
 * ルートハブへの標準（ハブ固有でない）コントロールメッセージを偽造する.
 *
 * @param req
 *      偽造するルートハブへの標準リクエスト.
 *
 * @return
 *      リクエストの処理が成功した場合は ::USB_STATUS_SUCCESS;
 *      そうでない場合は ::USB_STATUS_UNSUPPORTED_REQUEST などの
 *      ::usb_status_t エラーコード
 */
static usb_status_t
dwc_root_hub_standard_request(struct usb_xfer_request *req)
{
    uint16_t len;
    const struct usb_control_setup_data *setup = &req->setup_data;

    switch (setup->bRequest)
    {
        case USB_DEVICE_REQUEST_GET_STATUS:
            len = min(setup->wLength, sizeof(root_hub_device_status));
            memcpy(req->recvbuf, &root_hub_device_status, len);
            req->actual_size = len;
            return USB_STATUS_SUCCESS;

        case USB_DEVICE_REQUEST_SET_ADDRESS:
            return USB_STATUS_SUCCESS;

        case USB_DEVICE_REQUEST_GET_DESCRIPTOR:
            switch ((setup->wValue >> 8)) /* Switch on descriptor type */
            {
                case USB_DESCRIPTOR_TYPE_DEVICE:
                    len = min(setup->wLength, root_hub_device_descriptor.bLength);
                    memcpy(req->recvbuf, &root_hub_device_descriptor, len);
                    req->actual_size = len;
                    return USB_STATUS_SUCCESS;
                case USB_DESCRIPTOR_TYPE_CONFIGURATION:
                    len = min(setup->wLength,
                              root_hub_configuration.configuration.wTotalLength);
                    memcpy(req->recvbuf, &root_hub_configuration, len);
                    req->actual_size = len;
                    return USB_STATUS_SUCCESS;
                case USB_DESCRIPTOR_TYPE_STRING:
                    /* Index of string descriptor is in low byte of wValue */
                    if ((setup->wValue & 0xff) < ARRAY_LEN(root_hub_strings))
                    {
                        const struct usb_string_descriptor *desc =
                                root_hub_strings[setup->wValue & 0xff];
                        len = min(setup->wLength, desc->bLength);
                        memcpy(req->recvbuf, desc, len);
                        req->actual_size = len;
                        return USB_STATUS_SUCCESS;
                    }
                    return USB_STATUS_UNSUPPORTED_REQUEST;
            }
            return USB_STATUS_UNSUPPORTED_REQUEST;

        case USB_DEVICE_REQUEST_GET_CONFIGURATION:
            if (setup->wLength >= 1)
            {
                *(uint8_t*)req->recvbuf = req->dev->configuration;
                req->actual_size = 1;
            }
            return USB_STATUS_SUCCESS;

        case USB_DEVICE_REQUEST_SET_CONFIGURATION:
            if (setup->wValue <= 1)
            {
                return USB_STATUS_SUCCESS;
            }
    }
    return USB_STATUS_UNSUPPORTED_REQUEST;
}

/**
 * （USB標準形式の） <code>struct ::usb_hub_status</code> にルートハブの現在の
 * ステータスを設定する.
 *
 * @param status
 *      設定するハブステータス構造体
 */
static void
dwc_get_root_hub_status(struct usb_hub_status *status)
{
    status->wHubStatus = 0;
    status->wHubChange = 0;
    status->local_power = 1;
}

/**
 * ルートハブに接続されたポートへのSetPortFeatureリクエストを処理する.
 */
static usb_status_t
dwc_set_host_port_feature(enum usb_port_feature feature)
{
    switch (feature)
    {
        case USB_PORT_POWER:
            dwc_power_on_host_port();
            return USB_STATUS_SUCCESS;
        case USB_PORT_RESET:
            dwc_reset_host_port();
            return USB_STATUS_SUCCESS;
        default:
            return USB_STATUS_UNSUPPORTED_REQUEST;
    }
    return USB_STATUS_UNSUPPORTED_REQUEST;
}

/**
 * ルートハブに接続されたポートへのClearPortFeatureリクエストを処理する.
 */
static usb_status_t
dwc_clear_host_port_feature(enum usb_port_feature feature)
{
    switch (feature)
    {
        case USB_C_PORT_CONNECTION:
            host_port_status.connected_changed = 0;
            break;
        case USB_C_PORT_ENABLE:
            host_port_status.enabled_changed = 0;
            break;
        case USB_C_PORT_SUSPEND:
            host_port_status.suspended_changed = 0;
            break;
        case USB_C_PORT_OVER_CURRENT:
            host_port_status.overcurrent_changed = 0;
            break;
        case USB_C_PORT_RESET:
            host_port_status.reset_changed = 0;
            break;
        default:
            return USB_STATUS_UNSUPPORTED_REQUEST;
    }
    return USB_STATUS_SUCCESS;
}

/**
 * ルートハブへのハブクラス固有のコントロールメッセージを偽造する.
 *
 * @param req
 *      偽造するルートハブへのハブ固有リクエスト.
 *
 * @return
 *      リクエストの処理が成功した場合は ::USB_STATUS_SUCCESS;
 *      そうでない場合は ::USB_STATUS_UNSUPPORTED_REQUEST などの
 *      ::usb_status_t エラーコード
 */
static usb_status_t
dwc_root_hub_class_request(struct usb_xfer_request *req)
{
    uint16_t len;
    const struct usb_control_setup_data *setup = &req->setup_data;
    switch (setup->bRequest)
    {
        case USB_HUB_REQUEST_GET_DESCRIPTOR:
            switch ((setup->wValue >> 8)) /* Switch on descriptor type */
            {
                case USB_DESCRIPTOR_TYPE_HUB:
                    /* GetHubDescriptor (11.24.2) */
                    len = min(setup->wLength, root_hub_hub_descriptor.bDescLength);
                    memcpy(req->recvbuf, &root_hub_hub_descriptor, len);
                    req->actual_size = len;
                    return USB_STATUS_SUCCESS;
            }
            return USB_STATUS_UNSUPPORTED_REQUEST;
        case USB_HUB_REQUEST_GET_STATUS:
            switch (setup->bmRequestType & USB_BMREQUESTTYPE_RECIPIENT_MASK)
            {
                case USB_BMREQUESTTYPE_RECIPIENT_DEVICE:
                    /* GetHubStatus (11.24.2) */
                    if (setup->wLength >= sizeof(struct usb_hub_status))
                    {
                        dwc_get_root_hub_status(req->recvbuf);
                        req->actual_size = sizeof(struct usb_hub_status);
                        return USB_STATUS_SUCCESS;
                    }
                    return USB_STATUS_UNSUPPORTED_REQUEST;

                case USB_BMREQUESTTYPE_RECIPIENT_OTHER:
                    /* GetPortStatus (11.24.2) */
                    if (setup->wLength >= sizeof(struct usb_port_status))
                    {
                        memcpy(req->recvbuf, &host_port_status,
                               sizeof(struct usb_port_status));
                        req->actual_size = sizeof(struct usb_port_status);
                        return USB_STATUS_SUCCESS;
                    }
                    return USB_STATUS_UNSUPPORTED_REQUEST;
            }
            return USB_STATUS_UNSUPPORTED_REQUEST;

        case USB_HUB_REQUEST_SET_FEATURE:
            switch (setup->bmRequestType & USB_BMREQUESTTYPE_RECIPIENT_MASK)
            {
                case USB_BMREQUESTTYPE_RECIPIENT_DEVICE:
                    /* SetHubFeature (11.24.2) */
                    /* TODO */
                    return USB_STATUS_UNSUPPORTED_REQUEST;

                case USB_BMREQUESTTYPE_RECIPIENT_OTHER:
                    /* SetPortFeature (11.24.2) */
                    return dwc_set_host_port_feature(setup->wValue);
            }
            return USB_STATUS_UNSUPPORTED_REQUEST;

        case USB_HUB_REQUEST_CLEAR_FEATURE:
            switch (setup->bmRequestType & USB_BMREQUESTTYPE_RECIPIENT_MASK)
            {
                case USB_BMREQUESTTYPE_RECIPIENT_DEVICE:
                    /* ClearHubFeature (11.24.2) */
                    /* TODO */
                    return USB_STATUS_UNSUPPORTED_REQUEST;

                case USB_BMREQUESTTYPE_RECIPIENT_OTHER:
                    /* ClearPortFeature (11.24.2) */
                    return dwc_clear_host_port_feature(setup->wValue);
            }
            return USB_STATUS_UNSUPPORTED_REQUEST;
    }
    return USB_STATUS_UNSUPPORTED_REQUEST;
}

/**
 * ルートハブとの間のコントロール転送を偽造する.
 */
static usb_status_t
dwc_root_hub_control_msg(struct usb_xfer_request *req)
{
    switch (req->setup_data.bmRequestType & USB_BMREQUESTTYPE_TYPE_MASK)
    {
        case USB_BMREQUESTTYPE_TYPE_STANDARD:
            return dwc_root_hub_standard_request(req);
        case USB_BMREQUESTTYPE_TYPE_CLASS:
            return dwc_root_hub_class_request(req);
    }
    return USB_STATUS_UNSUPPORTED_REQUEST;
}

/**
 * るーとはぶへのリクエストを偽装する.
 */
static void
dwc_process_root_hub_request(struct usb_xfer_request *req)
{
    if (req->endpoint_desc == NULL)
    {
        /* デフォルトコントロールエンドポイントとの間のコントロール転送  */
        usb_debug("Simulating request to root hub's default endpoint\n");
        req->status = dwc_root_hub_control_msg(req);
        usb_complete_xfer(req);
    }
    else
    {
        /* ステータス変化エンドポイントからのインターラプト転送リクエスト。
         * 一度に1リクエストだけしか発行できないと仮定している */
        usb_debug("Posting status change request to root hub\n");
        root_hub_status_change_request = req;
        if (host_port_status.wPortChange != 0)
        {
            dwc_host_port_status_changed();
        }
    }
}

/**
 * USB上の低レベル転送を開始する.
 *
 * @param chan
 *      転送を開始するホストチャネルのインデックス
 * @param req
 *      次のトランザクション用のUSBリクエストセットアップ
 */
static void
dwc_channel_start_transaction(uint chan, struct usb_xfer_request *req)
{
    volatile struct dwc_host_channel *chanptr = &regs->host_channels[chan];
    union dwc_host_channel_split_control split_control;
    union dwc_host_channel_characteristics characteristics;
    union dwc_host_channel_interrupts interrupt_mask;
    uint next_frame;
    irqmask im;

    im = disable();

    /* 保留中の割り込みをクリアする  */
    chanptr->interrupt_mask.val = 0;
    chanptr->interrupts.val = 0xffffffff;

    /* このトランザクションがスプリットトランザクションの最後の部分であるか否かをセットする  */
    split_control = chanptr->split_control;
    split_control.complete_split = req->complete_split;
    chanptr->split_control = split_control;

    /* odd_frameをセットして、チャネルを有効にする  */

    next_frame = (regs->host_frame_number & 0xffff) + 1;

    if (!split_control.complete_split)
    {
        req->csplit_retries = 0;
    }
    characteristics = chanptr->characteristics;
    characteristics.odd_frame = next_frame & 1;
    characteristics.channel_enable = 1;
    chanptr->characteristics = characteristics;

    /* ソフトウェアが転送に対してアクションが必要なときに
     * dwc_interrupt_handler() が呼び出されるようにチャネルの
     * 割り込みマスクを必要な値にセットする。さらに、この
     * チャネルからの割り込みがホスト全チャネル割り込みマスク
     * レジスタで有効になっていることを確認する。注: ここで
     * さらに多くのチャネルの割り込みを有効にした場合は、
     * 停止されているチャネル以外の割り込みを考慮して
     * dwc_interrupt_handler() を変更する必要がある。
     */
    interrupt_mask.val = 0;
    interrupt_mask.channel_halted = 1;
    chanptr->interrupt_mask = interrupt_mask;
    regs->host_channels_interrupt_mask |= 1 << chan;

    restore(im);
}

/**
 * DesignWare Hi-Speed USB 2.0 OTG ControllerのチャネルでUSB転送を
 * 開始または再開する.
 *
 * これを行うためにソフトウェアはさまざまなレジスタに書き込むことに
 * より、USB上の一連の低レベルトランザクションのパラメータをDWCに
 * 与える必要がある。ここで使用するレジスタに関する詳細なドキュメントは
 * dwc_regs::dwc_host_channel の宣言に記載されている。
 *
 * @param chan
 *      転送を開始するホストチャネルのインデックス
 * @param req
 *      開始するUSB転送
 */
static void
dwc_channel_start_xfer(uint chan, struct usb_xfer_request *req)
{
    volatile struct dwc_host_channel *chanptr;
    union dwc_host_channel_characteristics characteristics;
    union dwc_host_channel_split_control split_control;
    union dwc_host_channel_transfer transfer;
    void *data;

    chanptr = &regs->host_channels[chan];
    characteristics.val = 0;
    split_control.val = 0;
    transfer.val = 0;
    req->short_attempt = 0;

    /* エンドポイント番号、エンドポイントタイプ、最大パケット長、
     * フレームあたりのパケット数を決定する */
    if (req->endpoint_desc != NULL)
    {
        /* 円とポイントは明示的に指定されている。エンドポイント
         * ディスクリプタから必要な情報を取得する */
        characteristics.endpoint_number =
                                    req->endpoint_desc->bEndpointAddress & 0xf;
        characteristics.endpoint_type =
                                    req->endpoint_desc->bmAttributes & 0x3;
        characteristics.max_packet_size =
                                    req->endpoint_desc->wMaxPacketSize & 0x7ff;
        characteristics.packets_per_frame = 1;
        if (req->dev->speed == USB_SPEED_HIGH)
        {
            characteristics.packets_per_frame +=
                        ((req->endpoint_desc->wMaxPacketSize >> 11) & 0x3);
        }
    }
    else
    {
        /* デフォルトコントロールエンドポイント。エンドポイント番号、
         * エンドポイントタイプ、フレームあたりのパケット数は事前に決定
         * されているが、最大パケットサイズはデバイスディスクリプタにある。  */
        characteristics.endpoint_number = 0;
        characteristics.endpoint_type = USB_TRANSFER_TYPE_CONTROL;
        characteristics.max_packet_size = req->dev->descriptor.bMaxPacketSize0;
        characteristics.packets_per_frame = 1;
    }

    /* エンドポイントの方向、データポインタ、データサイズ、
     * 初期パッケージIDを決定する。コントロール転送ではコントロール転送の
     * フェーズを考慮する必要がある。  */
    if (characteristics.endpoint_type == USB_TRANSFER_TYPE_CONTROL)
    {
        switch (req->control_phase)
        {
            case 0: /* SETUPフェーズ */
                usb_dev_debug(req->dev, "Starting SETUP transaction\n");
                characteristics.endpoint_direction = USB_DIRECTION_OUT;
                data = &req->setup_data;
                transfer.size = sizeof(struct usb_control_setup_data);
                transfer.packet_id = DWC_USB_PID_SETUP;
                break;

            case 1: /* DATAフェーズ */
                usb_dev_debug(req->dev, "Starting DATA transactions\n");
                characteristics.endpoint_direction =
                                        req->setup_data.bmRequestType >> 7;
                /* 一部完了した転送の再開であるかを慎重に考慮する必要がある  */
                data = req->recvbuf + req->actual_size;
                transfer.size = req->size - req->actual_size;
                if (req->actual_size == 0)
                {
                    /* DATAフェースの最初のトランザクション: パケットIDにDATA1を使う  */
                    transfer.packet_id = DWC_USB_PID_DATA1;
                }
                else
                {
                    /* DATAフェースのその後のトランザクション: 保存しておいたパケットIDを
                     * 復元する（DATA0かDATA1）  */
                    transfer.packet_id = req->next_data_pid;
                }
                break;

            default: /* STATUSフェーズ */
                usb_dev_debug(req->dev, "Starting STATUS transaction\n");
                /* STATUSトランザクションの方向はDATAトランザクションの方向の
                 * 反対である。DATAトランザクションがなかった場合はデバイスから
                 * ホストの方向である。 */
                if ((req->setup_data.bmRequestType >> 7) == USB_DIRECTION_OUT ||
                    req->setup_data.wLength == 0)
                {
                    characteristics.endpoint_direction = USB_DIRECTION_IN;
                }
                else
                {
                    characteristics.endpoint_direction = USB_DIRECTION_OUT;
                }
                /* STATUSトランザクションはデータバッファを持たない。
                 * しかし、DATA1パケットIDを使用する必要がある。 */
                data = NULL;
                transfer.size = 0;
                transfer.packet_id = DWC_USB_PID_DATA1;
                break;
        }
    }
    else /* コントロール転送以外の転送を開始、または再開する */
    {
        characteristics.endpoint_direction =
                    req->endpoint_desc->bEndpointAddress >> 7;

        /* コントロール転送のDATAフェーズの場合と同じように、一部完了した転送の
         * 再開であるかを慎重に考慮する必要がある  */
        data = req->recvbuf + req->actual_size;
        transfer.size = req->size - req->actual_size;
        /* このハードウェアは1（マイクロ）フレームにフィットするデータ
         * （エンドポイントが許容するフレームあたりの最大パケット数に
         * エンドポイントが許容する最大パケットサイズをかけたもの）以上の
         * データを持つインターラプト転送の開始を受け入れない。 */
        if (characteristics.endpoint_type == USB_TRANSFER_TYPE_INTERRUPT &&
            transfer.size > characteristics.packets_per_frame *
                            characteristics.max_packet_size)
        {
            transfer.size = characteristics.packets_per_frame *
                            characteristics.max_packet_size;
            req->short_attempt = 1;
        }
        transfer.packet_id = req->next_data_pid;
    }

    /* デバイスアドレスをセットする  */
    characteristics.device_address = req->dev->address;

    /* LSまたはFSのデバイスと通信する場合、スプリットコントロールレジスタを
     * プログラムする。また、転送しようとするサイズを最大パケットサイズに制限する。
     * 転送はスプリットの完了を待機するために延期される可能性が高い
     * （そのため、後で別のチャネルで再スケジュールされる可能性がある）からである。
     * 最後に、LSデバイスと通信する場合は、Channel Characteristicsレジスタに
     * low_speed フラグをセットする。*/
    if (req->dev->speed != USB_SPEED_HIGH)
    {
        /* どのハブがTransaction Translatorとして動作しているかを決定する */
        struct usb_device *tt_hub;
        uint tt_hub_port;

        tt_hub = req->dev;
        do {
            tt_hub_port = tt_hub->port_number;
            tt_hub = tt_hub->parent;
        } while (tt_hub->speed != USB_SPEED_HIGH);

        split_control.port_address = tt_hub_port - 1;
        split_control.hub_address = tt_hub->address;
        split_control.split_enable = 1;

        if (transfer.size > characteristics.max_packet_size)
        {
            transfer.size = characteristics.max_packet_size;
            req->short_attempt = 1;
        }

        if (req->dev->speed == USB_SPEED_LOW)
        {
            characteristics.low_speed = 1;
        }
    }

    /* DMAバッファを設定する  */
    if (IS_WORD_ALIGNED(data))
    {
        /* ワードアライメントされている場合はソースから、または
         * ディスティネーションに直接DMAすることができる  */
        chanptr->dma_address = (uint32_t)data;
    }
    else
    {
        /* 実際のソースまたはディスティネーションがワードアラインされて
         * いないため、DMA用に代替バッファを使用する必要がある。転送
         * しようとしているサイズがこの代替バッファより大きい場合、
         * 適合する最大のパケット数に制限する。 */
        chanptr->dma_address = (uint32_t)aligned_bufs[chan];
        if (transfer.size > sizeof(aligned_bufs[chan]))
        {
            transfer.size = sizeof(aligned_bufs[chan]) -
                            (sizeof(aligned_bufs[chan]) %
                              characteristics.max_packet_size);
            req->short_attempt = 1;
        }
        /* OUTエンドポイントでは送信するデータをDMAバッファにコピーする */
        if (characteristics.endpoint_direction == USB_DIRECTION_OUT)
        {
            memcpy(aligned_bufs[chan], data, transfer.size);
        }
    }

    /* 送受信する次のデータチャンクの開始位置へのポインタをセットする
     * （上で代替バッファが選択された場合、ハードウェアが使用する
     * 実際のDMAアドレスとは異なる場合がある）。  */
    req->cur_data_ptr = data;

    /* この転送用に設定するパケット数を計算する  */
    transfer.packet_count = DIV_ROUND_UP(transfer.size,
                                         characteristics.max_packet_size);
    if (transfer.packet_count == 0)
    {
        /* ハードウェアはたとえゼロ長の転送であっても、少なくても1パケットを
         * 指定することを要求する */
        transfer.packet_count = 1;
    }

    /* 転送を試みる実際のサイズとパケット数を記録する  */
    req->attempted_size = transfer.size;
    req->attempted_bytes_remaining = transfer.size;
    req->attempted_packets_remaining = transfer.packet_count;

    /* 割り込みハンドラが見つけることができる場所にこの保留リクエスを保存する  */
    channel_pending_xfers[chan] = req;

    usb_dev_debug(req->dev, "Setting up transactions on channel %u:\n"
                  "\t\tmax_packet_size=%u, "
                  "endpoint_number=%u, endpoint_direction=%s,\n"
                  "\t\tlow_speed=%u, endpoint_type=%s, device_address=%u,\n\t\t"
                  "size=%u, packet_count=%u, packet_id=%u, split_enable=%u, "
                  "complete_split=%u\n",
                  chan,
                  characteristics.max_packet_size,
                  characteristics.endpoint_number,
                  usb_direction_to_string(characteristics.endpoint_direction),
                  characteristics.low_speed,
                  usb_transfer_type_to_string(characteristics.endpoint_type),
                  characteristics.device_address,
                  transfer.size,
                  transfer.packet_count,
                  transfer.packet_id,
                  split_control.split_enable,
                  req->complete_split);

    /* 適切なチャネルのレジスタを実施にプログラムする  */
    chanptr->characteristics = characteristics;
    chanptr->split_control   = split_control;
    chanptr->transfer        = transfer;

    /* チャネルを有効にしてUSB転送を開始する。この後、この転送を処理するために
     * 次に実行されるコードは、ホストコントローラがこのチャネルに関する割り込みを
     ? 発行した後の dwc_handle_channel_halted_interrupt() にある。 */
    dwc_channel_start_transaction(chan, req);
}

/**
 * defer_xfer() で作成されるスレッド用のスレッドプロシジャ.
 *
 * このスレッドのインスタンスは usb_free_xfer_request() でキルされる。
 *
 * @param req
 *      遅延するUSB転送リクエスト
 *
 * @return
 *      今スレッドは復帰しない
 */
static thread
defer_xfer_thread(struct usb_xfer_request *req)
{
    uint interval_ms;
    uint chan;

    if (req->dev->speed == USB_SPEED_HIGH)
    {
        interval_ms = (1 << (req->endpoint_desc->bInterval - 1)) /
                              USB_UFRAMES_PER_MS;
    }
    else
    {
        interval_ms = req->endpoint_desc->bInterval / USB_FRAMES_PER_MS;
    }
    if (interval_ms <= 0)
    {
        interval_ms = 1;
    }
    for (;;)
    {
        wait(req->deferer_thread_sema);

#if START_SPLIT_INTR_TRANSFERS_ON_SOF
        if (req->need_sof)
        {
            union dwc_core_interrupts intr_mask;
            irqmask im;

            usb_dev_debug(req->dev,
                          "Waiting for start-of-frame\n");

            im = disable();
            chan = dwc_get_free_channel();
            channel_pending_xfers[chan] = req;
            sofwait |= 1 << chan;
            intr_mask = regs->core_interrupt_mask;
            intr_mask.sof_intr = 1;
            regs->core_interrupt_mask = intr_mask;

            receive();

            dwc_channel_start_xfer(chan, req);
            req->need_sof = 0;
            restore(im);
        }
        else
#endif /* START_SPLIT_INTR_TRANSFERS_ON_SOF */
        {
            usb_dev_debug(req->dev,
                          "Waiting %u ms to start xfer again\n", interval_ms);
            sleep(interval_ms);
            chan = dwc_get_free_channel();
            dwc_channel_start_xfer(chan, req);
        }
    }
    return SYSERR;
}

/**
 * エンドポイントに利用可能なデータがないためUSB転送を後で再試行する
 * 必要がある場合に呼び出される.
 *
 * 周期転送（インターラプトエンドポイントのポーリングなど）の場合、
 * 転送を再試行しなければならない正確な時間はエンドポイントディス
 * クリプタのbIntervalメンバで指定される。LS/HSデバイスでは
 * bIntervalは次のポーリングまで待つミリ秒数を指定し、HSデバイスでは
 * 次のポーリングまで待つミリ秒数の2のべき乗の指数（+ 1）を指定する。
 *
 * 実際に転送を遅延させるには、各転送をオンデマンドで作成される
 * スレッドに関連付ける。そのようなスレッドは単にループに入り、
 * 適切なミリ秒間 sleep() を呼び出し、その後、転送を再試行する。
 * リクエストが実際に発行され、遅延されるまでスレッドが何もしない
 * ようにするためにセマフォが必要である。
 *
 * 注意: このコードはハブやHIDデバイスなどのINインタラプトエンド
 * ポイントのスケジューリングポーリングに使用される。したがって、
 * これらのデバイスのステータス変化（ハブの場合）や新しい入力
 * （HIDデバイスの場合）のためのポーリングはソフトウェアで行われる。
 * このため、CPUを何度も起床させ、時間とエネルギーを浪費する。しかし、
 * USB2.0ではサポートしていないUSBデバイスをサスペンドする以外、
 * これを回避する方法はない。
 *
 * @param req
 *      遅延させるUSB転送
 *
 * @return
 *      遅延させたプロセスの開始に成功した場合は ::USB_STATUS_SUCCESS;
 *      それ以外は ::usb_status_t エラーコード.
 */
static usb_status_t
defer_xfer(struct usb_xfer_request *req)
{
    usb_dev_debug(req->dev, "Deferring transfer\n");
    if (SYSERR == req->deferer_thread_sema)
    {
        req->deferer_thread_sema = semcreate(0);
        if (SYSERR == req->deferer_thread_sema)
        {
            usb_dev_error(req->dev, "Can't create semaphore\n");
            return USB_STATUS_OUT_OF_MEMORY;
        }
    }
    if (BADTID == req->deferer_thread_tid)
    {
        req->deferer_thread_tid = create(defer_xfer_thread,
                                         DEFER_XFER_THREAD_STACK_SIZE,
                                         DEFER_XFER_THREAD_PRIORITY,
                                         DEFER_XFER_THREAD_NAME,
                                         1, req);
        if (SYSERR == ready(req->deferer_thread_tid, RESCHED_NO))
        {
            req->deferer_thread_tid = BADTID;
            usb_dev_error(req->dev,
                          "Can't create thread to service periodic transfer\n");
            return USB_STATUS_OUT_OF_MEMORY;
        }
    }
    signal(req->deferer_thread_sema);
    return USB_STATUS_SUCCESS;
}

/** 割り込み処理に使用する内部転送ステータスコード  */
enum dwc_intr_status {
    XFER_COMPLETE            = 0,
    XFER_FAILED              = 1,
    XFER_NEEDS_DEFERRAL      = 2,
    XFER_NEEDS_RESTART       = 3,
    XFER_NEEDS_TRANS_RESTART = 4,
};

/**
 * 明らかなエラーなしに停止しているチャネルを処理する.
 */
static enum dwc_intr_status
dwc_handle_normal_channel_halted(struct usb_xfer_request *req, uint chan,
                                 union dwc_host_channel_interrupts interrupts)
{
    volatile struct dwc_host_channel *chanptr = &regs->host_channels[chan];

    /* ハードウェアは期待通りに transfer.packet_count を更新して
     * いると思われるのでそれを確認して transfer.size （これは期待通りに
     * 更新されるとは限らない）を使うかどうか決める。 */
    uint packets_remaining   = chanptr->transfer.packet_count;
    uint packets_transferred = req->attempted_packets_remaining -
                               packets_remaining;

    usb_dev_debug(req->dev, "%u packets transferred on channel %u\n",
                  packets_transferred, chan);

    if (packets_transferred != 0)
    {
        uint bytes_transferred = 0;
        union dwc_host_channel_characteristics characteristics =
                                            chanptr->characteristics;
        uint max_packet_size = characteristics.max_packet_size;
        enum usb_direction dir = characteristics.endpoint_direction;
        enum usb_transfer_type type = characteristics.endpoint_type;

        /* 転送済みのバイト数を計算して必要ならDMAバッファから
         * データをコピーする  */

        if (dir == USB_DIRECTION_IN)
        {
            /* IN転送の場合、transfer.sizeフィールドは正当に更新
             * されていると思われる（そうでなければ、ショート
              * パケットの長さを決定することができないので
              * これも良いことである）。  */
            bytes_transferred = req->attempted_bytes_remaining -
                                chanptr->transfer.size;
            /* 必要ならDMAバッファからデータをコピーする */
            if (!IS_WORD_ALIGNED(req->cur_data_ptr))
            {
                memcpy(req->cur_data_ptr,
                       &aligned_bufs[chan][req->attempted_size -
                                           req->attempted_bytes_remaining],
                       bytes_transferred);
            }
        }
        else
        {
            /* OUT転送の transfer.size フィールドは正しく更新されて
             * いないので無視する。 */
            if (packets_transferred > 1)
            {
                /* 1つ以上のパケットが転送済みである。最後のパケット
                 * 以外は max_packet_size のはず  */
                bytes_transferred += max_packet_size * (packets_transferred - 1);
            }
            /* この転送試行で最後のパケットが送信された場合、その
             * サイズは転送試行の残りのサイズである。それ以外の
             * 場合はmax_packet_sizeである。  */
            if (packets_remaining == 0 &&
                (req->attempted_size % max_packet_size != 0 ||
                 req->attempted_size == 0))
            {
                bytes_transferred += req->attempted_size % max_packet_size;
            }
            else
            {
                bytes_transferred += max_packet_size;
            }
        }

        usb_dev_debug(req->dev, "Calculated %u bytes transferred\n",
                      bytes_transferred);

        /* 転送済みのパケットとバイトを記録する  */
        req->attempted_packets_remaining -= packets_transferred;
        req->attempted_bytes_remaining -= bytes_transferred;
        req->cur_data_ptr += bytes_transferred;

        /* （少なくともチャネルにプログラムされたデータまで）転送が
         * 完了したかチェックする  */
        if (req->attempted_packets_remaining == 0 ||
            (dir == USB_DIRECTION_IN &&
             bytes_transferred < packets_transferred * max_packet_size))
        {
            /* transfer_completed フラグはハードウェアによって
             * 設定されるはずであるが、他のタイミングでも設定される
             * ため本質的に意味がない（たとえば、スプリットトラン
             * ザクションが完了すると、転送すべきパケットがまだ残って
             * いても設定されるようだ）。  */
            if (!interrupts.transfer_completed)
            {
                usb_dev_error(req->dev, "transfer_completed flag not "
                              "set on channel %u as expected "
                              "(interrupts=0x%08x, transfer=0x%08x).\n", chan,
                              interrupts.val, chanptr->transfer.val);
                return XFER_FAILED;
            }

            /* 希望する転送サイズより小さい値をチャネルにプログラム
             * した場合（いくつかの理由のうちの1つは
             * dwc_channel_start_xfer() を参照）、それがインターラプト
             * 転送でない限り、転送を試み続ける。インターラプト転送の
             * 場合は、転送の試みは最大1回のはずであり、または（IN転送の
             * 場合） 試行より少ないバイト数が転送された場合はすでに
             * 転送が終了したことを示しているからである。  */
            if (req->short_attempt && req->attempted_bytes_remaining == 0 &&
                type != USB_TRANSFER_TYPE_INTERRUPT)
            {
                usb_dev_debug(req->dev,
                              "Starting next part of %u-byte transfer "
                              "after short attempt of %u bytes\n",
                              req->size, req->attempted_size);
                req->complete_split = 0;
                req->next_data_pid = chanptr->transfer.packet_id;
                if (!usb_is_control_request(req) || req->control_phase == 1)
                {
                    req->actual_size = req->cur_data_ptr - req->recvbuf;
                }
                return XFER_NEEDS_RESTART;
            }

            /* 他の転送と異なり、コントロール転送は複数のフェーズで
             * 構成されている。コントロール転送のSETUPまたはDATAフェーズが
             * 完了しただけの場合は、次のフェーズに進み、転送完了の
             * 信号は送らない。 */
            if (usb_is_control_request(req) && req->control_phase < 2)
            {
                /* CSPLIT フラグをリセッそする  */
                req->complete_split = 0;

                /* データフェースの完了直後の場合は転送したバイトを
                 * 記録する  */
                if (req->control_phase == 1)
                {
                    req->actual_size = req->cur_data_ptr - req->recvbuf;
                }

                /* 次のフェースに進む */
                req->control_phase++;

                /* 送受信するデータがない場合はDATAフェースをスキップする */
                if (req->control_phase == 1 && req->size == 0)
                {
                    req->control_phase++;
                }
                return XFER_NEEDS_RESTART;
            }

            /* 実際に転送が完了した（または、少なともリクエストより
             * 少ないバイト数で転送を完了したIN転送であった）。 */
            usb_dev_debug(req->dev, "Transfer completed on channel %u\n", chan);
            return XFER_COMPLETE;
        }
        else
        {
            /* 転送は完了していないので次のトランザクションを開始する  */

            /* スプリットとランざく書を行っている場合はCSPLITフラグを
             * 反転する */
            if (chanptr->split_control.split_enable)
            {
                req->complete_split ^= 1;
            }

            usb_dev_debug(req->dev, "Continuing transfer (complete_split=%u)\n",
                          req->complete_split);
            return XFER_NEEDS_TRANS_RESTART;
        }
    }
    else
    {
        /* パケットは転送されなかったがエラーフラグもセットされていない。
         * これはStart Splitトランザクションを実行した場合にのみ予想
         * されることであり、その場合は Complete Splitトランザクションに
         * 進む必要がある。さらに、ack_response_received フラグをチェック
         * する。このフラグはデバイスがStart Splitトランザクションを
         * 承認したことを示すためにセットされているはずだ。 */
        if (interrupts.ack_response_received &&
            chanptr->split_control.split_enable && !req->complete_split)
        {
            /* CSPLITを開始する */
            req->complete_split = 1;
            usb_dev_debug(req->dev, "Continuing transfer (complete_split=%u)\n",
                          req->complete_split);
            return XFER_NEEDS_TRANS_RESTART;
        }
        else
        {
            usb_dev_error(req->dev, "No packets transferred.\n");
            return XFER_FAILED;
        }
    }
}

/**
 * 指定されたチャネルのチャネル停止割り込みを処理する.  これは
 * dwc_channel_start_transaction() でチャネルを有効にした後で、
 * 対応するチャネルで停止割り込みがあった場合にいつでも発生する
 * 可能性がある。
 *
 * @param chan
 *      チャネル停止割り込みが発生したDWCホストチャネルのインデックス
 */
static void
dwc_handle_channel_halted_interrupt(uint chan)
{
    struct usb_xfer_request *req = channel_pending_xfers[chan];
    volatile struct dwc_host_channel *chanptr = &regs->host_channels[chan];
    union dwc_host_channel_interrupts interrupts = chanptr->interrupts;
    enum dwc_intr_status intr_status;

    usb_debug("Handling channel %u halted interrupt\n"
              "\t\t(interrupts pending: 0x%08x, characteristics=0x%08x, "
              "transfer=0x%08x)\n",
              chan, interrupts.val, chanptr->characteristics.val,
              chanptr->transfer.val);

    /* 割り込みの原因を決定する  */

    if (interrupts.stall_response_received || interrupts.ahb_error ||
        interrupts.transaction_error || interrupts.babble_error ||
        interrupts.excess_transaction_error || interrupts.frame_list_rollover ||
        (interrupts.nyet_response_received && !req->complete_split) ||
        (interrupts.data_toggle_error &&
         chanptr->characteristics.endpoint_direction == USB_DIRECTION_OUT))
    {
        /* エラーが発生した。エラーステータスを付けて転送を直ちに
         * 完了させる  */
        usb_dev_error(req->dev, "Transfer error on channel %u "
                      "(interrupts pending: 0x%08x, packet_count=%u)\n",
                      chan, interrupts.val, chanptr->transfer.packet_count);
        intr_status = XFER_FAILED;
    }
    else if (interrupts.frame_overrun)
    {
        /* フレームオーバーランで突発的に失敗したトランザクションは
         * 再スタートする。
         * TODO: これはなぜ起こるのか?  */
        usb_dev_debug(req->dev, "Frame overrun on channel %u; "
                      "restarting transaction\n", chan);
        intr_status = XFER_NEEDS_TRANS_RESTART;
    }
    else if (interrupts.nyet_response_received)
    {
        /* スプリットトランザクションを実行する際、デバイスがNYET
         * パケットを送信した。後でCSPLITを再試行する。特殊なケースと
         * して、NYETを何回も受信した場合は、スプリットトラン
         * ザクション全体を再スタートする（明らかに、フレームオーバー
         * ランやその他の理由で、トランザクションを再試行するまで
         * 無限にNYETが発行されることがあるようだ）。  */
        usb_dev_debug(req->dev, "NYET response received on channel %u\n", chan);
        if (++req->csplit_retries >= 10)
        {
            usb_dev_debug(req->dev, "Restarting split transaction "
                          "(CSPLIT tried %u times)\n", req->csplit_retries);
            req->complete_split = FALSE;
        }
        intr_status = XFER_NEEDS_TRANS_RESTART;
    }
    else if (interrupts.nak_response_received)
    {
        /* デバイスがNAKパケットを送信した。これは、デバイスがこの時点で
         * 送信するデータがなかった場合に起こる。後で再試行する。特殊な
         * ケース: Complete Splitトランザクション中にNAKが送信された
         * 場合は Complete SplitではなくStart Splitで再スタートする。  */
        usb_dev_debug(req->dev, "NAK response received on channel %u\n", chan);
        intr_status = XFER_NEEDS_DEFERRAL;
        req->complete_split = FALSE;
    }
    else
    {
        /* 秋ならなエラーは発生していない */
        intr_status = dwc_handle_normal_channel_halted(req, chan, interrupts);
    }

#if START_SPLIT_INTR_TRANSFERS_ON_SOF
    if ((intr_status == XFER_NEEDS_RESTART ||
         intr_status == XFER_NEEDS_TRANS_RESTART) &&
        usb_is_interrupt_request(req) && req->dev->speed != USB_SPEED_HIGH &&
        !req->complete_split)
    {
        intr_status = XFER_NEEDS_DEFERRAL;
        req->need_sof = 1;
    }
#endif

    switch (intr_status)
    {
        case XFER_COMPLETE:
            req->status = USB_STATUS_SUCCESS;
            break;
        case XFER_FAILED:
            req->status = USB_STATUS_HARDWARE_ERROR;
            break;
        case XFER_NEEDS_DEFERRAL:
            break;
        case XFER_NEEDS_RESTART:
            dwc_channel_start_xfer(chan, req);
            return;
        case XFER_NEEDS_TRANS_RESTART:
            dwc_channel_start_transaction(chan, req);
            return;
    }

    /* 転送完了、転送中にエラー発生、後で転送を再試行する必要あり  */

    /* データパケットIDを保存  */
    req->next_data_pid = chanptr->transfer.packet_id;

    /* このチャネルの割り込みをっクリアして無効にする  */
    chanptr->interrupt_mask.val = 0;
    chanptr->interrupts.val = 0xffffffff;

    /* チャネルを解消する  */
    channel_pending_xfers[chan] = NULL;
    dwc_release_channel(chan);

    /* コントロール転送のDATAフェース以外を実行中の場合を覗いて
     * 実際の転送済みサイズをセットする  */
    if (!usb_is_control_request(req) || req->control_phase == 1)
    {
        req->actual_size = req->cur_data_ptr - req->recvbuf;
    }

    /* NAKまたはNYETを受信したためここに来た場合はリクエストを
     * 遅延させる。  */
    if (intr_status == XFER_NEEDS_DEFERRAL)
    {
        usb_status_t status = defer_xfer(req);
        if (status == USB_STATUS_SUCCESS)
        {
            return;
        }
        else
        {
            req->status = status;
        }
    }

    /* 転送が成功裏に完了した、あるいはエラーが発生したためにここに
     * 来た場合は、デバイスドライバが提供する完了コールバックを
     * 呼び出す */
    usb_complete_xfer(req);
}

/**
 * Synopsys DesignWare Hi-Speed USB 2.0 On-The-Go Controller（DWC）用の
 * 割り込みハンドラ関数である. この関数はこのドライバが明示的に有効にした
 * 割り込みが保留されている場合にのみ呼び出される。このハードウェアの
 * 割り込みの概要については dwc_setup_interrupts() のコメントを参照。
 */
static interrupt
dwc_interrupt_handler(void)
{
    /* この割り込みハンドラが終了する前に他のスレッドがスケジュール
     * されるのを防ぐために 'resdefer' をセットする。これはこの
     * 割り込みハンドラが繰り返し実行されるのを防ぐ。  */
    extern int resdefer;
    resdefer = 1;

    union dwc_core_interrupts interrupts = regs->core_interrupts;

#if START_SPLIT_INTR_TRANSFERS_ON_SOF
    if (interrupts.sof_intr)
    {
        /* Start of frame (SOF) 割り込みが発生した  */

        usb_debug("Received SOF intr (host_frame_number=0x%08x)\n",
                  regs->host_frame_number);
        if ((regs->host_frame_number & 0x7) != 6)
        {
            union dwc_core_interrupts tmp;

            if (sofwait != 0)
            {
                uint chan;

                /* SOFを待っているチャネルを1つ起床させる */

                chan = first_set_bit(sofwait);
                send(channel_pending_xfers[chan]->deferer_thread_tid, 0);
                sofwait &= ~(1 << chan);
            }

            /* もはや不要であればSOF割り込みを無効にする */
            if (sofwait == 0)
            {
                tmp = regs->core_interrupt_mask;
                tmp.sof_intr = 0;
                regs->core_interrupt_mask = tmp;
            }

            /* SOF割り込みをクリアする */
            tmp.val = 0;
            tmp.sof_intr = 1;
            regs->core_interrupts = tmp;
        }
    }
#endif /* START_SPLIT_INTR_TRANSFERS_ON_SOF */

    if (interrupts.host_channel_intr)
    {
        /* 1つ以上のチャネルに保留中の割り込みがある  */

        uint32_t chintr;
        uint chan;

        /* 対応するホストチャネルで割り込みが発生した場合、
         * "Host All Channels Interrupt Register"のビットがセット
         * される。設定されたビットをすべて処理する。 */
        chintr = regs->host_channels_interrupt;
        do
        {
            chan = first_set_bit(chintr);
            dwc_handle_channel_halted_interrupt(chan);
            chintr ^= (1 << chan);
        } while (chintr != 0);
    }
    if (interrupts.port_intr)
    {
        /* ホストポートのステータスが変化した。host_port_statusを
         * 更新する。 */

        union dwc_host_port_ctrlstatus hw_status = regs->host_port_ctrlstatus;

        usb_debug("Port interrupt detected: host_port_ctrlstatus=0x%08x\n",
                  hw_status.val);

        host_port_status.connected   = hw_status.connected;
        host_port_status.enabled     = hw_status.enabled;
        host_port_status.suspended   = hw_status.suspended;
        host_port_status.overcurrent = hw_status.overcurrent;
        host_port_status.reset       = hw_status.reset;
        host_port_status.powered     = hw_status.powered;
        host_port_status.low_speed_attached = (hw_status.speed == USB_SPEED_LOW);
        host_port_status.high_speed_attached = (hw_status.speed == USB_SPEED_HIGH);

        host_port_status.connected_changed   = hw_status.connected_changed;
        host_port_status.enabled_changed     = hw_status.enabled_changed;
        host_port_status.overcurrent_changed = hw_status.overcurrent_changed;

        /* "Write-clear"なHost Port Control and Statusレジスタに
         * 書き戻すことで割り込みをクリアする。ただし、特別なケースと
         * して、'enabled' には0を書き込まなければならない。そうでないと
         * ポートは自分自身を無効にしてしまうようだ。  */
        hw_status.enabled = 0;
        regs->host_port_ctrlstatus = hw_status;

        /* ルートハブにステータス変化リクエストが発行されている場合は
         * それを完了させる。  */
        dwc_host_port_status_changed();
    }

    /* 割り込みハンドラが何らかのスレッド（たとえば、USB転送の完了を
     * 待っているスレッド）を起床しようとした場合、現在実行中の
     * スレッドをリスケジュールする。 */
    if (--resdefer > 0)
    {
        resdefer = 0;
        resched();
    }
}

/**
 * Synopsys Designware USB 2.0 On-The-Go Controller（DWC）の割り込みを
 * 初期設定する。
 *
 * DWCには次のリストに示すようにいくつかのレベルの割り込みレジスタがある。
 * 各レベルにおいて "interrupt" レジスタの各ビットは保留中の割り込みの
 * 状態を示し（1は保留中を意味し、1を書き込むとクリアする）、"interrupt
 * mask" レジスタは同じ形式だが対応する割り込みをオン/オフするために
 * 使用する（1はオンで、1を書き込むとオン、0を書き込むとオフになる）。
 *
 *  - AHBコンフィギュレーションレジスタは、DWCハードウェアからのすべての
 *    割り込みを有効/無効にするために使用されるマスクビットを含んでいる。
 *  - "Core" 割り込みと割り込みマスクレジスタは、トップレベルの割り込みを
 *    制御する。たとえば、これらのレジスタの1ビットが全チャネルの
 *    割り込みに対応する。
 *  - "Host All Channels" 割り込みと割り込みマスクレジスタは、各チャネルの
 *    すべての割り込みを制御する。
 *  - "Channel" 割り込みと割り込みマスクレジスタは、各チャネルに1つずつ
 *    存在し、そのチャネルの個々の割り込みタイプを制御する。
 *
 * 割り込みはこれらのすべての場所で有効になっている場合にのみ発生すると
 * みなすことができる。さらに、割り込みのクリアは最下位レベルでしか
 * できないようである。たとえば、チャネル割り込みは上位レベルの
 * いずれかの割り込みレジスタではなく、個々のチャネル割り込みレジスタで
 * クリアする必要がある。
 *
 * 以上はDWC固有の割り込みレジスタしかカバーしていません。これらに加えて、
 * システムは割り込みを制御する別の方法を持っている。たとえば、BCM2835
 * (Raspberry Pi) では、DWCへの割り込み線は数10本あるうちの1つにすぎず、
 * 割り込みコントローラを使って有効/無効を切り替えることができる。以下の
 * コードでは、この割り込みラインを有効にし、ハンドラ関数を登録して、
 * 実際にDWCから割り込みを受けることができるようにしている。
 *
 * さらに、ARMプロセッサのCPSR、または、他のCPUにおいてそれに相当する
 * ものがある。つまり、USB転送が完了したときに割り込みを受けるには
 * 文字通り6か所で割り込みを有効にする必要がある。
 */
static void
dwc_setup_interrupts(void)
{
    union dwc_core_interrupts core_interrupt_mask;

    /* 保留中のコア割り込みをすべてクリアする  */
    regs->core_interrupt_mask.val = 0;
    regs->core_interrupts.val = 0xffffffff;

    /* コアホストチャネルとポート割り込みを有効にする  */
    core_interrupt_mask.val = 0;
    core_interrupt_mask.host_channel_intr = 1;
    core_interrupt_mask.port_intr = 1;
    regs->core_interrupt_mask = core_interrupt_mask;

    /* USBコントローラに通ずる割り込みラインを有効にし、
     * 割り込みハンドラを登録する  */
    interruptVector[IRQ_USB] = dwc_interrupt_handler;
    enable_irq(IRQ_USB);

    /* USBホストコントローラ全体の割り込みを有効にする（そう、これは
     * 今したことだ。しかし、これはホストコントローラ自身によって
     * 制御されている）。*/
    regs->ahb_configuration |= DWC_AHB_INTERRUPT_ENABLE;
}

/**
 * ホストコントローラドライバに発行されたがまだチャネルで開始されて
 * いないUSB転送リクエストのキュー
 */
static mailbox hcd_xfer_mailbox;

/**
 * USB転送リクエストスケジューラスレッド: このスレッドはスケジュー
 * リングが必要なUSB転送リクエストを繰り返し待ち、空きチャンネルを
 * 待ち、そのチャンネルで転送リクエストを開始する。これは帯域幅の
 * 要件やどのエンドポイントに対する転送であるかを考慮していないので
 * 明らかに非常に単純なスケジューラである。
 *
 * @return
 *      このスレッドは復帰しない
 */
static thread
dwc_schedule_xfer_requests(void)
{
    uint chan;
    struct usb_xfer_request *req;

    for (;;)
    {
        /* 次の転送リクエストを取得する  */
        req = (struct usb_xfer_request*)mailboxReceive(hcd_xfer_mailbox);
        if (is_root_hub(req->dev))
        {
            /* 特殊なケース: リクエストはルートハブ向け。偽装する */
            dwc_process_root_hub_request(req);
        }
        else
        {
            /* 通常のケース: いずれかのチャネルで転送をスケジュールする */
            chan = dwc_get_free_channel();
            dwc_channel_start_xfer(chan, req);
        }
    }
    return SYSERR;
}

/**
 * ホストチャネルの空き/使用中状況を管理するビットマスクとセマフォ、および、
 * 発行済のUSB転送リクエストを格納するキューを初期化し、USB転送リクエスト
 * スケジューラスレッドを起動する。
 */
static usb_status_t
dwc_start_xfer_scheduler(void)
{
    hcd_xfer_mailbox = mailboxAlloc(1024);
    if (SYSERR == hcd_xfer_mailbox)
    {
        return USB_STATUS_OUT_OF_MEMORY;
    }

    chfree_sema = semcreate(DWC_NUM_CHANNELS);
    if (SYSERR == chfree_sema)
    {
        mailboxFree(hcd_xfer_mailbox);
        return USB_STATUS_OUT_OF_MEMORY;
    }
    STATIC_ASSERT(DWC_NUM_CHANNELS <= 8 * sizeof(chfree));
    chfree = (1 << DWC_NUM_CHANNELS) - 1;

    dwc_xfer_scheduler_tid = create(dwc_schedule_xfer_requests,
                                    XFER_SCHEDULER_THREAD_STACK_SIZE,
                                    XFER_SCHEDULER_THREAD_PRIORITY,
                                    XFER_SCHEDULER_THREAD_NAME, 0);
    if (SYSERR == ready(dwc_xfer_scheduler_tid, RESCHED_NO))
    {
        semfree(chfree_sema);
        mailboxFree(hcd_xfer_mailbox);
        return USB_STATUS_OUT_OF_MEMORY;
    }
    return USB_STATUS_SUCCESS;
}

/* DesignWare Hi-Speed USB 2.0 On-The-Go Controller用の
 * hcd_start() の実装.  ホストコントローラドライバのこのインタフェースの
 * ドキュメントについては usb_hcdi.h を参照されたい。  */
usb_status_t
hcd_start(void)
{
    usb_status_t status;

    status = dwc_power_on();
    if (status != USB_STATUS_SUCCESS)
    {
        return status;
    }
    dwc_soft_reset();
    dwc_setup_dma_mode();
    dwc_setup_interrupts();
    status = dwc_start_xfer_scheduler();
    if (status != USB_STATUS_SUCCESS)
    {
        dwc_power_off();
    }
    return status;
}

/* DesignWare Hi-Speed USB 2.0 On-The-Go Controller用の
 * hcd_stop() の実装.  ホストコントローラドライバのこのインタフェースの
 * ドキュメントについては usb_hcdi.h を参照されたい。  */
void
hcd_stop(void)
{
    /* IRQラインとハンドラを無効にする  */
    disable_irq(IRQ_USB);
    interruptVector[IRQ_USB] = NULL;

    /* 転送スケジューラスレッドを停止する */
    kill(dwc_xfer_scheduler_tid);

    /* USB転送リクエストメールボックスを開放する  */
    mailboxFree(hcd_xfer_mailbox);

    /* 不要なセマフォを開放する  */
    semfree(chfree_sema);

    /* USBハードウェアの電源を切る  */
    dwc_power_off();
}

/* DesignWare Hi-Speed USB 2.0 On-The-Go Controller用の
 * hcd_submit_xfer_request() の実装.  ホストコントローラドライバのこのインタフェースの
 * ドキュメントについては usb_hcdi.h を参照されたい。  */
/**
 * @details
 *
 * このホストコントローラドライバは、このインタフェースを意図された
 * とおり、非同期に実装している。さらに、単純化されたスケジューリング
 * アルゴリズムを使用している。すなわち、転送リクエストは一つのキューに
 * 入れ、発行された順に実行している。再試行が必要な転送、たとえば、
 * NAK応答を受信した周期転送やComplete Splitトランザクションを行っている
 * 際にNYET応答を受信したスプリットトランザクションなどは、タイマーが
 * 切れたときにメインキューをショートカットする別のコードによって、
 * 適切な時間に再試行するようスケジュールされる。
 *
 * 次に何が起こるかは dwc_schedule_xfer_requests() を参照のこと。
 */
usb_status_t
hcd_submit_xfer_request(struct usb_xfer_request *req)
{
    mailboxSend(hcd_xfer_mailbox, (int)req);
    return USB_STATUS_SUCCESS;
}
