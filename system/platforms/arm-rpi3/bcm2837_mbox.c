/**
 * @file bcm2837_mbox.c
 *
 * BCM2837B0 (Pi 3 B+)用のメールボックス関数.
 */
/* Embedded Xinu Copyright (C) 2009, 2013, 2018. All rights reserved.
 *
 * Authors:     Patrick J. McGee
 *              Rade Latinovich
 */

#include <kernel.h>
#include <stdint.h>
#include <string.h>
#include "bcm2837_mbox.h"

/**
 * @ingroup bcm2837
 *
 * メールボックスにタグを追加する. メールボックスリクエストを構築する
 * 前にこのステップを踏む必要がある。
 * @param buffer    メールボックスバッファ
 * @param tag       渡すタグ
 * @param buflen    バッファ長
 * @param len       送信するデータ長
 * @param data      送信するデータ
 */
void add_mailbox_tag(volatile uint32_t* buffer, uint32_t tag, uint32_t buflen, uint32_t len, uint32_t* data) {
    volatile uint32_t* start = buffer + SLOT_TAGSTART;
    start[SLOT_TAG_ID] = tag;
    start[SLOT_TAG_BUFLEN] = buflen;
    start[SLOT_TAG_DATALEN] = len & 0x7FFFFFFF;
    uint32_t bufwords = buflen >> 2;

    if (0 == data) {
        for (int i = 0; i < bufwords; ++i) {
            start[SLOT_TAG_DATA + i] = 0;
        }
    } else {
        for (int i = 0; i < bufwords; ++i) {
            start[SLOT_TAG_DATA + i] = data[i];
        }
    }
    /* 後で上書きしない限り、終了タグ */
    start[SLOT_TAG_DATA+bufwords] = 0;
}

/**
 * @ingroup bcm2837
 *
 * 正しい長さ、スロットを初期化することでバッファを用意する.
 * @param mailbuffer    メールボックスバッファ
 */
void build_mailbox_request(volatile uint32_t* mailbuffer) {
    uint32_t tag_length = mailbuffer[MBOX_HEADER_LENGTH + SLOT_TAG_BUFLEN];
    uint32_t end = (MBOX_HEADER_LENGTH*4) + (TAG_HEADER_LENGTH*4) + tag_length;
    uint32_t overall_length = end + 4;
    mailbuffer[SLOT_OVERALL_LENGTH] = overall_length;
    mailbuffer[SLOT_RR] = RR_REQUEST;
}

/**
 * @ingroup bcm2837
 *
 * 対応するMACメールボックスタグを使用してMACアドレスを
 * 取得する.
 * @param mailbuffer    メールボックスバッファ
 */
void get_mac_mailbox(volatile uint32_t* mailbuffer){

    /* タグをロードしてバッファを構築する */
    add_mailbox_tag(mailbuffer, MBX_TAG_GET_MAC_ADDRESS, 8, 0, 0);
    build_mailbox_request(mailbuffer);

    /* メールボックスレジスタに書き込む */
    bcm2837_mailbox_write(8, (uint32_t)mailbuffer);
    bcm2837_mailbox_read(8);
}

/**
 * @ingroup bcm2837
 *
 * 対応するタグを使用してARMメモリを取得する.
 * @param mailbuffer    メールボックスバッファ
 */
void get_armmemory_mailbox(volatile uint32_t* mailbuffer){

    /* タグをロードしてバッファを構築する */
    add_mailbox_tag(mailbuffer, MBX_TAG_GET_ARM_MEMORY, 8, 0, 0);
    build_mailbox_request(mailbuffer);

    /* メールボックスレジスタに書き込む */
    bcm2837_mailbox_write(8, (uint32_t)mailbuffer);
    bcm2837_mailbox_read(8);
}

/**
 * @ingroup bcm2837
 *
 * 対応するタグを使用してボードシリアル番号を取得する.
 * @param mailbuffer    メールボックスバッファ
 */
void get_serial_mailbox(volatile uint32_t* mailbuffer){

    /* タグをロードしてバッファを構築する */
    add_mailbox_tag(mailbuffer, MBX_TAG_GET_BOARD_SERIAL, 8, 0, 0);
    build_mailbox_request(mailbuffer);

    /* メールボックスレジスタに書き込む */
    bcm2837_mailbox_write(8, (uint32_t)mailbuffer);
    bcm2837_mailbox_read(8);
}
