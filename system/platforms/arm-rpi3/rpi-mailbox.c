#include <stddef.h>
#include <stdint.h>
#include <system/arch/arm/rpi-mailbox.h>
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
static uint32_t rpi_mailbox_read(MAILBOX_CHANNEL channel)
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
static int rpi_mailbox_write(MAILBOX_CHANNEL channel, uint32_t message)
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
