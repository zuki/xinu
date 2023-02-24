#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "rpi-mailbox.h"
#include "bcm2837.h"
#include "interrupt.h"

/**
 * Raspberry Piメールボックスハードウェアレジスタ.
 *      packとalignは64ビットコンパイラでは必須
 */
struct __attribute__((__packed__, aligned(4))) MailBoxRegisters {
    const uint32_t Read0;           // 0x00         Read data from VC to ARM
    uint32_t Unused[3];             // 0x04-0x0F
    uint32_t Peek0;                 // 0x10
    uint32_t Sender0;               // 0x14
    uint32_t Status0;               // 0x18         Status of VC to ARM
    uint32_t Config0;               // 0x1C
    uint32_t Write1;                // 0x20         Write data from ARM to VC
    uint32_t Unused2[3];            // 0x24-0x2F
    uint32_t Peek1;                 // 0x30
    uint32_t Sender1;               // 0x34
    uint32_t Status1;               // 0x38         Status of ARM to VC
    uint32_t Config1;               // 0x3C
};

/**
 * Raspberry Piメールボックスアドレス
 *      https://github.com/raspberrypi/firmware/wiki 参照
 */
#define MAILBOX ((volatile __attribute__((aligned(4))) struct MailBoxRegisters*)(PERIPHERALS_BASE + 0xB880))

/* Raspberry Piメールボックス関数 */
/**
 * 指定されたチャンネルのメールボックスシステムのすべての保留中のデータを
 * 読み込む.
 *
 * @param channel 使用するメールボックスチャンネル
 * @return 成功した場合は読み取り値、失敗した場合（メールボックスから
 *   返された実際の有効な値の下位4ビットが0）は 0x1。
 */
uint32_t rpi_mailbox_read(MAILBOX_CHANNEL channel)
{
    uint32_t value;
    if (channel > MB_CHANNEL_GPU)
        return 0x01;
    do {
        do {
            value = MAILBOX->Status0;           // メールボックスステータス0を読み込む
        } while ((value & MAILBOX_EMPTY) != 0);    // データが現れるまで待機
        value = MAILBOX->Read0;                 // データを読み込む
    } while ((value & 0xF) != channel);         // 指定したチャンネルの応答か確認
    value &= ~(0xF);                            // チャンネルビットの下位4ビットをクリア
    return value;                               // 読み込んだデータを返す
}

/**
 * 指定されたチャンネルを通じてメッセージを送信する.
 *
 * @param channel 使用するメールボックスチャンネル
 * @param message 送信するデータブロックメッセージ
 * @return 成功した場合は1、失敗した場合は 0。
 */
int rpi_mailbox_write(MAILBOX_CHANNEL channel, uint32_t message)
{
    uint32_t value;
    if (channel > MB_CHANNEL_GPU)
        return 1;
    message &= ~(0xF);                      // チャンネルビットの下位4ビットをクリア
    message |= channel;                     // チャンネルビットをOR
    do {
        value = MAILBOX->Status1;           // メールボックスステータス1を読み込む
    } while ((value & MAILBOX_FULL) != 0);     // メールボックスがフルでなくなるまで待機
    MAILBOX->Write1 = message;              // メールボックスに値を書き込む
    return 0;
}

/**
 * 指定されたチャンネルのメールボックスシステムを通して指定されたデータ
 * ブロックメッセージの送信を実行する.
 * ただし、コンテキストスイッチされないようにする。これは必要である。
 * なぜなら、PIメールボックスメッセージが発行された場合、他のPIメールボックス
 * メッセージが送信される前にその応答を読まなければならないからである。
 *
 * @param channel メールボックスチャネル
 * @param msg 送信メッセージ
 * @return 成功の場合は OK、失敗の場合は SYSERR。
 */
int rpi_MailBoxAccess(uint32_t channel, uint32_t msg)
{
    int status = SYSERR;
    irqmask im = disable();
    if ( (rpi_mailbox_write(channel, msg) & 0xF) == 0x0) {
        uint32_t result = rpi_mailbox_read(channel);
        if ( (result & 0xF) == 0x0) status = OK;
    }
    restore(im);
    return status;
}

/**
 * ボードリビジョンの取得 (mailbox経由)
 */
uint32_t rpi_getModel(void)
{
    int retval;
    uint32_t  __attribute__((aligned(16))) msg[7] =
    {
        sizeof(msg),                    // Message size
        0,                              // Response will go here
        MAILBOX_TAG_GET_BOARD_REVISION, // 0x00010002 : ボードリビジョンの取得
        4,                              // 値バッファサイズ（バイト）
        0,                              // リクエストコード: リクエスト
        0,                              // Model response
        0                               // Tag end marker
    };

    retval = rpi_MailBoxAccess(MB_CHANNEL_TAGS, (uint32_t)&msg[0]);
    if (retval == OK)
    {
        return (msg[5]);                // ボードリビジョンを返す
    }
    return 0;                           // エラーの場合は0
}

/**
 * ボードシリアル番号の取得 (mailbox経由)
 */
uint64_t rpi_getserial(void)
{
    int retval;
    uint32_t  __attribute__((aligned(16))) msg[8] =
    {
        sizeof(msg),                    // Message size
        0,                              // Response will go here
        MAILBOX_TAG_GET_BOARD_SERIAL,   // 0x00010004 : ボードリビジョンの取得
        8,                              // 値バッファサイズ（バイト）
        0,                              // リクエストコード: リクエスト
        0,                              // シリアル番号の下位32ビット
        0,                              // シリアル番号の上位32ビット
        0                               // Tag end marker
    };

    retval = rpi_MailBoxAccess(MB_CHANNEL_TAGS, (uint32_t)&msg[0]);
    if (retval == OK)
    {
        return ((uint64_t)msg[6] << 32 | (uint64_t)msg[5]); // ボードシリアルの64ビットで返す
    }
    return 0UL;                           // エラーの場合は0
}

/**
 * ARMメモリ上限の取得 (mailbox経由)
 */
uint32_t rpi_LastARMAddr(void)
{
    int retval;
    uint32_t  __attribute__((aligned(16))) msg[8] =
    {
        sizeof(msg),                    // Message size
        0,                              // Response will go here
        MAILBOX_TAG_GET_ARM_MEMORY,     // 0x00010005 : ARMメモリの取得
        8,                              // 値バッファサイズ（バイト）
        0,                              // リクエストコード: リクエスト
        0,                              // ベースアドレス
        0,                              // メモリサイズ
        0                               // Tag end marker
    };

    retval = rpi_MailBoxAccess(MB_CHANNEL_TAGS, (uint32_t)&msg[0]);
    if (retval == OK)   // ARMメモリ情報の取得に成功
    {
        return (msg[5] + msg[6]);   // ベースアドレス + メモリサイズ
    }
    return 0;                       // エラーの場合はNULL
}

/**
 * MACアドレスをメールボックス経由で取得する
 */
uint32_t rpi_getmacaddr(uint8_t *macaddr)
{
    int retval;
    uint32_t  __attribute__((aligned(16))) msg[8] =
    {
        sizeof(msg),                    // Message size
        0,                              // Response will go here
        MAILBOX_TAG_GET_BOARD_MAC_ADDRESS, // 0x00010003 : MACアドレスの取得
        6,                              // 値バッファサイズ（バイト）
        0,                              // リクエストコード: リクエスト
        0,                              // MACアドレスの下位アドレス
        0,                              // MACアドレスの上位アドレス
        0,                              // Tag end marker
    };

    retval = rpi_MailBoxAccess(MB_CHANNEL_TAGS, (uint32_t)&msg[0]);
    if (retval == OK) {
        memcpy(macaddr, ((uint8_t *)msg) + 20, 6);
        return OK;
    }
    return 0;                           // エラーの場合は0
}

/**
 * クロックレートの設定 (mailbox経由)
 */
int rpi_setclockfreq(MB_CLOCK_ID clkid, uint32_t freq, uint32_t turbo)
{
    uint32_t  __attribute__((aligned(16))) msg[9] =
    {
        sizeof(msg),                    // Message size
        0,                              // Response will go here
        MAILBOX_TAG_SET_CLOCK_RATE,     // 0x00038002 : クロックレートの設定
        12,                             // 値バッファサイズ（バイト）
        0,                              // リクエストコード: リクエスト
        (uint32_t)clkid,                // クロックid
        freq,                           // 周波数
        turbo,                          // ターボ設定 (1: skip)
        0                               // Tag end marker
    };

    return rpi_MailBoxAccess(MB_CHANNEL_TAGS, (uint32_t)&msg[0]);
}

/**
 * クロックレートの取得 (mailbox経由)
 */
uint32_t rpi_getclockfreq(MB_CLOCK_ID clkid)
{
    int retval;
    uint32_t  __attribute__((aligned(16))) msg[8] =
    {
        sizeof(msg),                    // Message size
        0,                              // Response will go here
        MAILBOX_TAG_SET_CLOCK_RATE,     // 0x00038002 : クロックレートの設定
        8 ,                             // 値バッファサイズ（バイト）
        0,                              // リクエストコード: リクエスト
        (uint32_t)clkid,                // クロックid
        0,                              // 周波数
        0                               // Tag end marker
    };

    retval = rpi_MailBoxAccess(MB_CHANNEL_TAGS, (uint32_t)&msg[0]);
    if (retval == OK)
        return msg[6];
    return SYSERR;
}

/*
 * ペリフェラルの電源を制御する (mailbox経由)
 */
int rpi_setpower(MAILBOX_POWER_ID pwrid, uint32_t on)
{
    uint32_t  __attribute__((aligned(16))) msg[8] =
    {
        sizeof(msg),                    // Message size
        0,                              // Response will go here
        MAILBOX_TAG_SET_POWER_STATE,    // 0x00028001 : 電源状態の設定
        8,                              // 値バッファサイズ（バイト）
        0,                              // リクエストコード: リクエスト
        (uint32_t)pwrid,                // 電源ID
        (on | 0x2),                     // 1: Power on,  0: Power off (安定するまで待つ)
        0                               // Tag end marker
    };
    return rpi_MailBoxAccess(8, (uint32_t)&msg[0]);
}

/**
 * CPUのクロックレートを最大クロックレートに設定する (1.4GHz)
*/
void rpi_set_cpu_maxspeed(void)
{
    uint32_t  __attribute__((aligned(16))) msg[8] =
    {
        sizeof(msg),                    // Message size
        0,                              // Response will go here
        MAILBOX_TAG_GET_MAX_CLOCK_RATE, // 0x00030004 : ARM最大クロックレートを取得
        8,                              // 値バッファサイズ（バイト）
        0,                              // リクエストコード: リクエスト
        3,                              // ARMクロック
        0,                              // クロックレート
        0                               // Tag end marker
    };
    if (rpi_MailBoxAccess(MB_CHANNEL_TAGS, (uint32_t)&msg[0]) == OK)
    {
        uint32_t  __attribute__((aligned(16))) msg2[9] =
        {
            sizeof(msg2),                   // Message size
            0,                              // Response will go here
            MAILBOX_TAG_SET_CLOCK_RATE,     // 0x00038002 : ARM最大クロックレートを設定
            8,                              // 値バッファサイズ（バイト）
            0,                              // リクエストコード: リクエスト
            3,                              // ARMクロック
            msg[6],                         // クロックレート
            1,                              // ターボ設定をスキップ
            0                               // Tag end marker
        };
        if (rpi_MailBoxAccess(MB_CHANNEL_TAGS, (uint32_t)&msg[0]) == OK)
        {
            /* ARM speed taken to max */
        }
    }
}
