/**
 * @file usb_dwc_regs.h
 * @ingroup usbhcd
 *
 * DesignWare Hi-Speed USB 2.0 On-The-Go Controllerのレジスタ.
 *
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#ifndef _ARM_USB_DWC_REGS_H_
#define _ARM_USB_DWC_REGS_H_

#include <usb_util.h>

/**
 * @ingroup usbhcd
 * DWCホストのチャンネル数である. それぞれが独立のUSB転送に使用できる。
 * BCM2835 (Raspberry Pi)では8チャンネル使用できる。これはドキュメント
 * "BCM2835 ARM Peripherals" の201ページに記載されている。
 */
#define DWC_NUM_CHANNELS 8

/**
 * @ingroup usbhcd
 *
 * DesignWare Hi-Speed USB 2.0 On-The-Goコントローラのレジスタレイアウト.
 * これについての正式なドキュメントは存在しない。ただし、レジスタの位置
 * （とそのある程度の意味）はSynopsys社が提供したこのハードウェアの
 * Linuxドライバなど、他のコードで見つけることができる。
 *
 * ここではレジスタのすべてのビットを明示的に定義してはいない。なぜなら、
 * 大部分はドライバで使用されておらず、このファイルを複雑にするからである。
 * たとえば、サスペンド、ハイバネーション、OTGプロトコル、ホストモードでは
 * なくデバイスモードで動作するコアに特有の機能については記載していない。
 *
 * ドライバで使用しているビットやフィールドについては、それらが何をする
 * ものであるかを理解した上で、完全に文書化するように努めた。しかし、
 * 公式の文書にアクセスできないので、すべての情報が正しいことを保証する
 * ものではない。
 */
struct dwc_regs {

    /* 0x000 : GOTGCTL: OTGコントロール/ステータスレジスタ. */
    uint32_t otg_control;

    /* 0x004 : GOTGINT: OTG割り込みレジスタ. */
    uint32_t otg_interrupt;

    /**
     * 0x008 : GAHBCFG: AHBコンフィグレーションレジスタ.
     *
     * このレジスタはDWCとシステムの他のペリフェラルとのインタフェースを
     * 構成する  */
    uint32_t ahb_configuration;

/**
 * USBコントローラからの割り込みを有効にする（実際はアプリケーション全体の
 * 割り込みを有効にする: アンマスク）. デフォルトは無効（マスク）になっている。 */
#define DWC_AHB_INTERRUPT_ENABLE  (1 << 0)

/**
 * AHBコンフィグレーションレジスタのビット [4:1] はBroadcomによりBCM2835ように
 * 再定義されている。したがって、こののフラグはBCM2835でのみ妥当である。
 *
 * このビットは次のように定義されている。
 *
 *    1 = 未処理のAXI書き込みがすべて完了するのを待ってからDMAの完了を
 *        （内部で）シグナリングする。
 *    0 = 待たない
 *
 * Linuxドライバがそうしているのでこのビットをセットしたが、動作に違いは
 * 見られなかった。
 */
#define BCM_DWC_AHB_AXI_WAIT      (1 << 4)

/**
 * AHBコンフィグレーションレジスタのこのビットに1を書き込むとUSBコントローラ
 * （実際はコア）がDMAモードで実行するようになる。デフォルトは無効 (0)
 */
#define DWC_AHB_DMA_ENABLE        (1 << 5)

    /* 0x00c : GUSBCFG: USBコンフィグレーションレジスタ. */
    uint32_t core_usb_configuration;


    /**
     * 0x010 : GRSTCTL: コアリセットレジスタ.
     *
     * ソフトウェアはこのレジスタを使ってコア内部の様々なハードウェアを
     * リセットすることができる。
     */
    uint32_t core_reset;

    /**
     * AHB Master Idle: AHBマスタステートマシンがIDLE条件にあることを示す
     */
#define DWC_AHB_MASTER_IDLE (1 << 31)

    /**
     * コアリセットレジスタのこのビットに1を書き込むとソフトウェアリセットが
     * 開始される。このビットはリセットが完了すると自動的にクリアされる。
     */
#define DWC_SOFT_RESET      (1 << 0)

    /**
     * 0x014 : GINTSTS: コア割り込みレジスタ.
     *
     * このレジスタは保留されているトップレベルのDWC割り込みの状態を含んでいる。
     * 1は割り込みが保留されていることを、0は保留されていないことを意味する。
     *
     * 少なくともport_intrとhost_channel_intrについては、このレジスタに書き込む
     * のではなく、他のどこかで割り込みをクリアしなければならないことに注意。
     */
    union dwc_core_interrupts {
        uint32_t val;
        struct {
            uint32_t stuff             : 3;

            /**
             * Start of Frame.  TODO
             */
            uint32_t sof_intr          : 1;

            uint32_t morestuff         : 20;

            /**
             * ホストポートの状態が変更された。ソフトウェアはHPRTでホスト
             * ポートの現在の状態を調べて、変更のあったステータスのフラグを
             * クリアする必要がある。
             */
            uint32_t port_intr         : 1;     /* Bit 24 */

            /**
             * チャネル割り込みが発生した。ソフトウェアはHAINTでどのチャネルで
             * 割り込みが保留されているか調べて、その割り込みを処理して、その
             * チャネルの割り込みをクリアする必要がある。
             */
            uint32_t host_channel_intr : 1;     /* Bit 25 */

            uint32_t evenmorestuff     : 6;
        };
    } core_interrupts;

    /**
     * 0x018 : GINTMSK: コア割り込みマスクレジスタ.
     *
     * このレジスタはGINTSTSと同じ形式であり、対応する割り込みを有効 (1) または
     * 無効 (0) にする。リセット後の初期状態はすべて 0
     */
    union dwc_core_interrupts core_interrupt_mask;

    /* 0x01c : GRXSTSR: 受信ステータスキュー読み込み */
    uint32_t receive_status;

    /* 0x020 : GRXSTSP: 受信ステータスキュー読み込みとポップアウト */
    uint32_t receive_status_pop;

    /**
     * 0x024 : GRXFSIZ: 受信FIFOサイズレジスタ.
     *
     * This register contains the size of the Receive FIFO, in 4-byte words.
     *
     * This register must be set by software before using the controller; see
     * the note in the documentation for the hwcfg3 register about configuring
     * the dynamic FIFOs.
     */
    uint32_t rx_fifo_size;

    /**
     * 0x028 : Non Periodic Transmit FIFO Size Register.
     *
     * The low 16 bits of this register contain the offset of the Nonperiodic
     * Transmit FIFO, in 4-byte words, from the start of the memory reserved by
     * the controller for dynamic FIFOs.  The high 16 bits of this register
     * contain its size, in 4-byte words.
     *
     * This register must be set by software before using the controller; see
     * the note in the documentation for the hwcfg3 register about configuring
     * the dynamic FIFOs.
     */
    uint32_t nonperiodic_tx_fifo_size;

    /* 0x02c : Non Periodic Transmit FIFO/Queue Status Register */
    uint32_t nonperiodic_tx_fifo_status;

    /* 0x030 */
    uint32_t i2c_control;

    /* 0x034 */
    uint32_t phy_vendor_control;

    /* 0x038 */
    uint32_t gpio;

    /* 0x03c */
    uint32_t user_id;

    /* 0x040 */
    uint32_t vendor_id;

    /* 0x044 */
    uint32_t hwcfg1;

    /* 0x048 */
    uint32_t hwcfg2;

    /**
     * 0x04c : Hardware Configuration 3 Register.
     *
     * The high 16 bits of this read-only register contain the maximum total
     * size, in words, of the dynamic FIFOs (Rx, Nonperiodic Tx, and Periodic
     * Tx).  Software must set up these three dynamic FIFOs in the rx_fifo_size,
     * nonperiodic_tx_fifo_size, and host_periodic_tx_fifo_size registers such
     * that their total size does not exceed this maximum total size and no
     * FIFOs overlap.
     *
     * Note: Software must explicitly configure the dynamic FIFOs even if the
     * controller is operating in DMA mode, since the default values for the
     * FIFO sizes and offsets may be invalid.  For example, in Broadcom's
     * instantiation of this controller for the BCM2835, only 4080 words are
     * available for dynamic FIFOs, but the dynamic FIFO sizes are set to 4096,
     * 32, and 0, which are invalid as they add up to more than 4080.  <b>IF YOU
     * DO NOT DO THIS YOU WILL GET SILENT MEMORY CORRUPTION</b>.
     *
     * The low 16 bits of this register contain various flags that are not
     * documented here as we don't use any in our driver.
     */
    uint32_t hwcfg3;

    /* 0x050 */
    uint32_t hwcfg4;

    /* 0x054 */
    uint32_t core_lpm_configuration;

    /* 0x058 */
    uint32_t global_powerDn;

    /* 0x05c */
    uint32_t global_fifo_config;

    /* 0x060 */
    uint32_t adp_control;

    /* 0x064 */
    uint32_t reserved_0x64[39];

    /**
     * 0x100 : Host Periodic Transmit FIFO Size Register.
     *
     * The low 16 bits of this register configure the offset of the Periodic
     * Transmit FIFO, in 4-byte words, from the start of the memory reserved by
     * the controller for dynamic FIFOs.  The high 16 bits of this register
     * configure its size, in 4-byte words.
     *
     * This register should be set by software before using the controller; see
     * the note in the documentation for the hwcfg3 register about configuring
     * the dynamic FIFOs.
     */
    uint32_t host_periodic_tx_fifo_size;

    /* TODO */
    uint32_t stuff[191];

    /**
     * @name ホストレジスタ
     *
     * このポイントから始まるレジスタは「ホスト」レジスタとみなされる。
     * これらはOTG（On-The-Go）プロトコルの「ホスト」パートで使用される。
     * OTGはこのハードウェアがUSBホストまたはUSBデバイスとして動作することを
     * 可能にする。このドライバではホストだけを扱うのでデバイスに対ソウル
     * レジスタは宣言しない
     */
    /**@{*/

    /* 0x400 */
    uint32_t host_configuration;

    /* 0x404 */
    uint32_t host_frame_interval;

    /* 0x408 */
    uint32_t host_frame_number;

    /* 0x40c */
    uint32_t host_reserved_0x40c;

    /* 0x410 */
    uint32_t host_fifo_status;

    /**
     * 0x414 : Host All Channels Interrupt Register.
     *
     * This register contains a bit for each host channel that indicates whether
     * an interrupt has occurred on that host channel.  You cannot clear the
     * interrupts by writing to this register; use the channel-specific
     * interrupt registers instead.
     */
    uint32_t host_channels_interrupt;

    /**
     * 0x418 : Host All Channels Interrupt Mask Register.
     *
     * Same format as the Host All Channels Interrupt Register, but a 1 in this
     * register indicates that the corresponding host channel interrupt is
     * enabled.  Software can change this register.  Defaults to all 0's after a
     * reset.
     */
    uint32_t host_channels_interrupt_mask;

    /* 0x41c */
    uint32_t host_frame_list;

    /* 0x420 */
    uint32_t host_reserved_0x420[8];

    /**
     * 0x440 : Host Port Control and Status Register.
     *
     * This register provides the information needed to respond to status
     * queries about the "host port", which is the port that is logically
     * attached to the root hub.
     *
     * When changing this register, software must read its value, then clear the
     * enabled, connected_changed, enabled_changed, and overcurrent_changed
     * members to avoid changing them, as those particular bits are cleared by
     * writing 1.
     */
    union dwc_host_port_ctrlstatus {
        uint32_t val;
        struct {
            /**
             *  1: a device is connected to this port.
             *  0: no device is connected to this port.
             *
             *  Changed by hardware only.
             */
            uint32_t connected : 1; /* Bit 0 */

            /**
             * Set by hardware when connected bit changes.  Software can write
             * 1 to acknowledge and clear.  The setting of this bit by hardware
             * generates an interrupt that can be enabled by setting port_intr
             * in the core_interrupt_mask register.
             */
            uint32_t connected_changed : 1; /* Bit 1 */

            /**
             * 1: port is enabled.
             * 0: port is disabled.
             *
             * Note: the host port is enabled by default after it is reset.
             *
             * Note: Writing 1 here appears to disable the port.
             */
            uint32_t enabled : 1; /* Bit 2 */

            /**
             * Set by hardware when enabled bit changes.  Software can write 1
             * to acknowledge and clear.  The setting of this bit by hardware
             * generates an interrupt that can be enabled by setting port_intr
             * in the core_interrupt_mask register.
             */
            uint32_t enabled_changed : 1; /* Bit 3 */

            /**
             * 1: overcurrent condition active on this port
             * 0: no overcurrent condition active on this port
             *
             * Changed by hardware only.
             */
            uint32_t overcurrent : 1; /* Bit 4 */

            /**
             * Set by hardware when the overcurrent bit changes.  The software
             * can write 1 to acknowledge and clear.  The setting of this bit by
             * hardware generates the interrupt that can be enabled by setting
             * port_intr in the core_interrupt_mask register.
             */
            uint32_t overcurrent_changed : 1; /* Bit 5 */

            /**
             * Set by software to set resume signalling.
             */
            uint32_t resume : 1; /* Bit 6 */

            /**
             * Set by software to suspend the port.
             */
            uint32_t suspended : 1; /* Bit 7 */

            /**
             * Software can set this to start a reset on this port.  Software
             * must clear this after waiting 60 milliseconds for the reset is
             * complete.
             */
            uint32_t reset : 1; /* Bit 8 */

            uint32_t reserved : 1; /* Bit 9 */

            /**
             * Current logic of data lines (10: logic of D+; 11: logic of D-).
             *
             * Changed by hardware only.
             */
            uint32_t line_status : 2; /* Bits 10-11*/

            /**
             * 1: port is powered.
             * 0: port is not powered.
             *
             * Software can change this bit to power on (1) or power off (0)
             * the port.
             */
            uint32_t powered : 1; /* Bit 12 */

            uint32_t test_control : 4; /* Bits 13-16 */

            /**
             * Speed of attached device (if any).  This should only be
             * considered meaningful if the connected bit is set.
             *
             * 00: high speed; 01: full speed; 10: low speed
             *
             * Changed by hardware only.
             */
            uint32_t speed : 2; /* Bits 17-18 */

            uint32_t reserved2 : 13; /* Bits 19-32 */

        };
    } host_port_ctrlstatus;

    uint32_t host_reserved_0x444[47];

    /**
     * 0x500 : Array of host channels.  Each host channel can be used to
     * execute an independent USB transfer or transaction simultaneously.  A USB
     * transfer may consist of multiple transactions, or packets.  To avoid
     * having to re-program the channel, it may be useful to use one channel for
     * all transactions of a transfer before allowing other transfers to be
     * scheduled on it.
     */
    struct dwc_host_channel {

        /**
         * Channel Characteristics Register -
         *
         * Contains various fields that must be set to prepare this channel for
         * a transfer to or from a particular endpoint on a particular USB
         * device.
         *
         * This register only needs to be programmed one time when doing a
         * transfer, regardless of how many packets it consists of, unless the
         * channel is re-programmed for a different transfer or the transfer is
         * moved to a different channel.
         */
        union dwc_host_channel_characteristics {
            uint32_t val;
            struct {
                /**
                 * Maximum packet size the endpoint is capable of sending or
                 * receiving.  Must be programmed by software before starting
                 * the transfer.
                 */
                uint32_t max_packet_size     : 11; /* Bits 0-10  */

                /**
                 * Endpoint number (low 4 bits of bEndpointAddress).  Must be
                 * programmed by software before starting the transfer.
                 */
                uint32_t endpoint_number     : 4;  /* Bits 11-14 */

                /**
                 * Endpoint direction (high bit of bEndpointAddress).  Must be
                 * programmed by software before starting the transfer.
                 */
                uint32_t endpoint_direction  : 1;  /* Bit  15    */

                uint32_t reserved            : 1;  /* Bit  16    */

                /**
                 * 1 when the device being communicated with is attached at low
                 * speed; 0 otherwise.  Must be programmed by software before
                 * starting the transfer.
                 */
                uint32_t low_speed           : 1;  /* Bit  17    */

                /**
                 * Endpoint type (low 2 bits of bmAttributes).  Must be
                 * programmed by software before starting the transfer.
                 */
                uint32_t endpoint_type       : 2;  /* Bits 18-19 */

                /**
                 * Maximum number of transactions that can be executed per
                 * microframe as part of this transfer.  Normally 1, but should
                 * be set to 1 + (bits 11 and 12 of wMaxPacketSize) for
                 * high-speed interrupt and isochronous endpoints.  Must be
                 * programmed by software before starting the transfer.
                 */
                uint32_t packets_per_frame   : 2;  /* Bits 20-21 */

                /**
                 * USB device address of the device on which the endpoint is
                 * located.  Must be programmed by software before starting the
                 * transfer.
                 */
                uint32_t device_address      : 7;  /* Bits 22-28 */

                /**
                 * Just before enabling the channel (for all transactions),
                 * software needs to set this to the opposite of the low bit of
                 * the host_frame_number register.  Otherwise the hardware will
                 * issue frame overrun errors on some transactions.  TODO: what
                 * exactly does this do?
                 */
                uint32_t odd_frame           : 1;  /* Bit  29    */

                /**
                 * Software can set this to 1 to halt the channel.  Not needed
                 * during normal operation as the channel halts automatically
                 * when a transaction completes or an error occurs.
                 */
                uint32_t channel_disable     : 1;  /* Bit  30    */

                /**
                 * Software can set this to 1 to enable the channel, thereby
                 * actually starting the transaction on the USB.  This must only
                 * be done after the characteristics, split_control, and
                 * transfer registers, and possibly other registers (depending
                 * on the transfer) have been programmed.
                 */
                uint32_t channel_enable      : 1;  /* Bit  31    */
            };
        } characteristics;

        /**
         * Channel Split Control Register -
         *
         * This register is used to set up Split Transactions for communicating
         * with low or full-speed devices attached to a high-speed hub.  When
         * doing so, set split_enable to 1 and the other fields as documented.
         * Otherwise, software must clear this register before starting the
         * transfer.
         *
         * Like the Channel Characteristics register, this register only
         * needs to be programmed one time if the channel is enabled multiple
         * times to send all the packets of a single transfer.
         */
        union dwc_host_channel_split_control {
            uint32_t val;
            struct {
                /**
                 * 0-based index of the port on the high-speed hub on which the
                 * low or full-speed device is attached.
                 */
                uint32_t port_address          : 7;  /* Bits 0-6   */

                /**
                 * USB device address of the high-speed hub that acts as the
                 * Transaction Translator for this low or full-speed device.
                 * This is not necessarily the hub the device is physically
                 * connected to, since that could be a full-speed or low-speed
                 * hub.  Instead, software must walk up the USB device tree
                 * (towards the root hub) until a high-speed hub is found and
                 * use its device address here.
                 */
                uint32_t hub_address           : 7;  /* Bits 7-13  */

                /**
                 * TODO: what exactly does this do?
                 */
                uint32_t transaction_position  : 2;  /* Bits 14-15 */

                /**
                 *  0: Do a Start Split transaction
                 *  1: Do a Complete Split transaction.
                 *
                 * When split transactions are enabled, this must be programmed
                 * by software before enabling the channel.  Note that you must
                 * begin with a Start Split transaction and alternate this bit
                 * for each transaction until the transfer is complete.
                 */
                uint32_t complete_split        : 1;  /* Bit  16    */

                uint32_t reserved              : 14; /* Bits 17-30 */

                /**
                 * Set to 1 to enable Split Transactions.
                 */
                uint32_t split_enable          : 1;  /* Bit  31    */
            };
        } split_control;

        /**
         * Channel Interrupts Register -
         *
         * Bitmask of status conditions that have occurred on this channel.
         *
         * These bits can be used with or without "real" interrupts.  To have
         * the CPU get a real interrupt when one of these bits gets set, set the
         * appropriate bit in the interrupt_mask, and also ensure that
         * interrupts from the channel are enabled in the
         * host_channels_interrupt_mask register, channel interrupts overall are
         * enabled in the core_interrupt_mask register, and interrupts from the
         * DWC hardware overall are enabled in the ahb_configuration register
         * and by any system-specific interrupt controller.
         */
        union dwc_host_channel_interrupts {
            uint32_t val;
            struct {
                /**
                 * The requested USB transfer has successfully completed.
                 *
                 * Exceptions and caveats:
                 *
                 * - When doing split transactions, this bit will be set after a
                 *   Complete Split transaction has finished, even though the
                 *   overall transfer may not actually be complete.
                 *
                 * - The transfer will only be complete up to the extent that
                 *   data was programmed into the channel.  For example, control
                 *   transfers have 3 phases, each of which must be programmed
                 *   into the channel separately.  This flag will be set after
                 *   each of these phases has successfully completed.
                 *
                 * - An OUT transfer is otherwise considered complete when
                 *   exactly the requested number of bytes of data have been
                 *   successfully transferred, while an IN transfer is otherwise
                 *   considered complete when exactly the requested number of
                 *   bytes of data have been successfully transferred or a
                 *   shorter-than-expected packet was received.
                 */
                uint32_t transfer_completed       : 1;  /* Bit 0     */

                /**
                 * The channel has halted.  After this bit has been set, the
                 * channel sits idle and nothing else will happen until software
                 * takes action.
                 *
                 * Channels may halt for several reasons.  From our experience
                 * these cover all possible situations in which software needs
                 * to take action, so this is the only channel interrupt that
                 * actually needs to be enabled.  At least in DMA mode, the
                 * controller to some extent will act autonomously to complete
                 * transfers and only issue this interrupt when software needs
                 * to take action.
                 *
                 * Situations in which a channel will halt include but probably
                 * are not limited to:
                 *
                 * - The transfer has completed, thereby setting the
                 *   transfer_completed flag as documented above.
                 *
                 * - A Start Split or Complete Split transaction has finished.
                 *
                 * - The hub sent a NYET packet when trying to execute a
                 *   Complete Split transaction, thereby signalling that the
                 *   Split transaction is not yet complete.
                 *
                 * - The device sent a NAK packet, thereby signalling it had no
                 *   data to send at the time, when trying to execute an IN
                 *   interrupt transfer.
                 *
                 * - One of several errors has occurred, such as an AHB error,
                 *   data toggle error, tranasction error, stall condition, or
                 *   frame overrun error.
                 */
                uint32_t channel_halted           : 1;  /* Bit 1     */

                /**
                 * An error occurred on the ARM Advanced High-Performance Bus
                 * (AHB).
                 */
                uint32_t ahb_error                : 1;  /* Bit 2     */

                /**
                 * The device issued a STALL handshake packet (endpoint is
                 * halted or control pipe request is not supported).
                 */
                uint32_t stall_response_received  : 1;  /* Bit 3     */

                /**
                 * The device issued a NAK handshake packet (receiving device
                 * cannot accept data or transmitting device cannot send data).
                 *
                 * The channel will halt with this bit set when performing an IN
                 * transfer from an interrupt endpoint that has no data to send.
                 * As this requires software intervention to restart the
                 * channel, this means that polling of interrupt endpoints (e.g.
                 * on hubs and HID devices) must be done in software, even if
                 * the actual transactions themselves are interrupt-driven.
                 */
                uint32_t nak_response_received    : 1;  /* Bit 4     */

                /**
                 * The device issued an ACK handshake packet (receiving device
                 * acknowledged error-free packet).
                 */
                uint32_t ack_response_received    : 1;  /* Bit 5     */

                /**
                 * The device issued a NYET handshake packet.
                 */
                uint32_t nyet_response_received   : 1;  /* Bit 6     */

                /**
                 * From our experience this seems to usually indicate that
                 * software programmed the channel incorrectly.
                 */
                uint32_t transaction_error        : 1;  /* Bit 7     */

                /**
                 * Unexpected bus activity occurred.
                 */
                uint32_t babble_error             : 1;  /* Bit 8     */

                /**
                 * TODO
                 */
                uint32_t frame_overrun            : 1;  /* Bit 9     */

                /**
                 * When issuing a series of DATA transactions to an endpoint,
                 * the correct DATA0 or DATA1 packet ID was not specified in the
                 * packet_id member of the transfer register.
                 */
                uint32_t data_toggle_error        : 1;  /* Bit 10    */

                uint32_t buffer_not_available     : 1;  /* Bit 11    */
                uint32_t excess_transaction_error : 1;  /* Bit 12    */
                uint32_t frame_list_rollover      : 1;  /* Bit 13    */
                uint32_t reserved                 : 18; /* Bits 14-31 */
            };
        } interrupts;

        /**
         * Channel Interrupts Mask Register -
         *
         * This has the same format as the Channel Interrupts Register, but
         * software uses this to enable (1) or disable (0) the corresponding
         * interrupt.  Defaults to all 0's after a reset.
         */
        union dwc_host_channel_interrupts interrupt_mask;

        /**
         * Channel Transfer Register:
         *
         * Used to store additional information about the transfer.  This must
         * be programmed before beginning the transfer.
         */
        union dwc_host_channel_transfer {
            uint32_t val;
            struct {
                /**
                 * Size of the data to send or receive, in bytes.  Software must
                 * program this before beginning the transfer.  This can be
                 * greater than the maximum packet length.
                 *
                 * For IN transfers, the hardware decrements this field for each
                 * packet received by the number of bytes received.  For split
                 * transactions, the decrement happens after the Complete Split
                 * rather than the Start Split.  Software can subtract this
                 * field from the original transfer size in order to determine
                 * the number of bytes received at any given point, including
                 * when the transfer has encountered an error or has completed
                 * with either the full size or a short size.
                 *
                 * For OUT transfers, the hardware does not update this field as
                 * expected.  It will not be decremented when data is
                 * transmitted, at least not in every case; hence, software
                 * cannot rely on its value to indicate how many bytes of data
                 * have been transmitted so far.  Instead, software must inspect
                 * the packet_count field and assume that all data was
                 * transmitted if packet_count is 0, or that the amount of data
                 * transmitted is equal to the endpoint's maximum packet size
                 * times [the original packet count minus packet_count] if
                 * packet_count is nonzero.
                 */
                uint32_t size         : 19; /* Bits 0-18  */

                /**
                 * Number of packets left to transmit or maximum number of
                 * packets left to receive.  Software must program this before
                 * beginning the transfer.  The packet count is calculated as
                 * the size divided by the maximum packet size, rounded up to
                 * the nearest whole packet.  As a special case, if the transfer
                 * size is 0 bytes, the packet count must be set to 1.
                 *
                 * The hardware will decrement this register when a packet is
                 * successfully sent or received.  In the case of split
                 * transactions, this happens after the Complete Split rather
                 * than after the Start Split.  If the final received packet of
                 * an IN transfer is short, it is still counted.
                 */
                uint32_t packet_count : 10; /* Bits 19-28 */

                /**
                 * High 2 bits of the Packet ID used in the USB protocol.
                 *
                 * When performing the SETUP phase of a control transfer,
                 * specify 0x3 here to generate the needed SETUP token.
                 *
                 * When performing the DATA phase of a control transfer,
                 * initially specify 0x2 here to begin the DATA packets with the
                 * needed DATA1 Packet ID.
                 *
                 * When performing the STATUS phase of a control transfer,
                 * specify 0x2 here to generate the neeed DATA1 Packet ID.
                 *
                 * When starting a bulk, isochronous, or interrupt transfer,
                 * specify 0x0 here to generate the needed DATA0 Packet ID.
                 *
                 * In the case of a transfer consisting of multiple DATA
                 * packets, the hardware will update this field with the Packet
                 * ID to use for the next packet.  This field therefore only
                 * needs to be re-programmed if the transfer is moved to a
                 * different channel or the channel is re-used before the
                 * transfer is complete.  When doing so, software must save this
                 * field so that it can be re-programmed correctly.
                 */
                uint32_t packet_id    : 2;  /* Bits 29-30 */

                /**
                 * TODO
                 */
                uint32_t do_ping      : 1;  /* Bit  31    */
            };
        } transfer;

        /**
         * Channel DMA Address Register -
         *
         * Word-aligned address at which the hardware will read or write data
         * using Direct Memory Access.  This must be programmed before beginning
         * the transfer, unless the size of the data to send or receive is 0.
         * The hardware will increment this address by the number of bytes
         * successfully received or sent, which will correspond to the size
         * decrease in transfer.size.
         *
         * Note: DMA must be enabled in the AHB Configuration Register before
         * this register can be used.  Otherwise, the hardware is considered to
         * be in Slave mode and must be controlled a different way, which we do
         * not use in our driver and do not attempt to document.
         *
         * BCM2835-specific note:  Theoretically, addresses written to this
         * register must be bus addresses, not ARM physical addresses.  However,
         * in our experience the behavior is the same when simply using ARM
         * physical addresses.
         */
        uint32_t dma_address;

        uint32_t reserved_1;
        uint32_t reserved_2;
    } host_channels[DWC_NUM_CHANNELS];

    uint32_t host_reserved_after_channels[(0x800 - 0x500 -
                        (DWC_NUM_CHANNELS * sizeof(struct dwc_host_channel))) /
                         sizeof(uint32_t)];

    /**@}*/

    /* 0x800 */

    uint32_t reserved_0x800[(0xe00 - 0x800) / sizeof(uint32_t)];

    /* 0xe00 : Power and Clock Gating Control Register */
    uint32_t power;
};

/* Make sure the registers are declared correctly.  This is dummy code that will
 * be compiled into nothing.  */
static inline void _dwc_check_regs(void)
{
    STATIC_ASSERT(offsetof(struct dwc_regs, vendor_id) == 0x40);
    STATIC_ASSERT(offsetof(struct dwc_regs, host_periodic_tx_fifo_size) == 0x100);
    STATIC_ASSERT(offsetof(struct dwc_regs, host_configuration) == 0x400);
    STATIC_ASSERT(offsetof(struct dwc_regs, host_port_ctrlstatus) == 0x440);
    STATIC_ASSERT(offsetof(struct dwc_regs, reserved_0x800) == 0x800);
    STATIC_ASSERT(offsetof(struct dwc_regs, power) == 0xe00);
}

#endif /* _USB_DWC_REGS_H_ */
