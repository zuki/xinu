/**
 * @file platforminit.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <stdint.h>
#include <stddef.h>
#include <platform.h>
#include <string.h>
#include <framebuffer.h>
#include <usbkbd.h>
#include <stdio.h>
#include <usb_util.h>
#include <system/arch/arm/rpi-mailbox.h>
#include "bcm2837.h"

const struct atag *atags_ptr = (void*)-1;

/* -------------------------------------------------------------------------
 *  GPIO FSEL定義 ... BCM2835.PDF マニュアル p.92 参照
 * ------------------------------------------------------------------------- */
typedef enum {
    GPIO_INPUT    = 0b000,              // 0
    GPIO_OUTPUT   = 0b001,              // 1
    GPIO_ALTFUNC5 = 0b010,              // 2
    GPIO_ALTFUNC4 = 0b011,              // 3
    GPIO_ALTFUNC0 = 0b100,              // 4
    GPIO_ALTFUNC1 = 0b101,              // 5
    GPIO_ALTFUNC2 = 0b110,              // 6
    GPIO_ALTFUNC3 = 0b111,              // 7
} GPIOMODE;

/**
 * ボードリビジョンの取得 (mailbox経由)
 */
static uint32_t RPi_getModel(void)
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
static uint64_t RPi_getserial(void)
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
static uint32_t RPi_LastARMAddr(void)
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
 * CPUのクロックレートを最大クロックレートに設定する (1.4GHz)
*/
static void RPi_set_cpu_maxspeed(void)
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

/**
 * Raspberry Pi 3B+ 固有の情報を初期化する.
 * @return OK
 */
int platforminit(void)
{
    uint64_t serial;
    RPI_MODEL_ID id;

    strlcpy(platform.family, "BCM2837", PLT_STRMAX);
    strlcpy(platform.name, "Raspberry Pi 3B+", PLT_STRMAX);
    platform.minaddr = 0;
    platform.maxaddr = (void *)RPi_LastARMAddr();
    platform.clkfreq = 1000000;
    serial = RPi_getserial();
    platform.serial_low = (uint32_t)serial;
    platform.serial_high = (uint32_t)(serial >> 32);
    id.Raw32 = RPi_getModel();
    platform.model_id = id.model;
    RPi_set_cpu_maxspeed();
    return OK;
}

/**
 * ペリフェラルの電源を制御する (mailbox経由)
 */
int bcm2835_setpower(enum board_power_feature feature, bool on)
{
    int retval;
    uint32_t  __attribute__((aligned(16))) msg[8] =
    {
        sizeof(msg),                    // Message size
        0,                              // Response will go here
        MAILBOX_TAG_SET_POWER_STATE,    // 0x00028001 : 電源状態の設定
        8,                              // 値バッファサイズ（バイト）
        0,                              // リクエストコード: リクエスト
        (uint32_t)feature,              // 電源ID
        (uint32_t)on,                   // 1: Power on,  0: Power off
        0                               // Tag end marker
    };
    retval = rpi_MailBoxAccess(8, (uint32_t)&msg[0]);
    return (retval == OK) ? USB_STATUS_SUCCESS : USB_STATUS_HARDWARE_ERROR;
}
