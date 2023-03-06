/**
 * @file tftpRecvPackets.c
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <tftp.h>
#include <device.h>
#include <thread.h>

/**
 * @ingroup tftp
 *
 * UDPデバイスからTFTPパケットを繰り返し読み取り、バッファに格納して他のスレッドに
 * 提供するスレッドプロシージャ. これはread()にタイムアウトサポートがないことの
 * 回避策である。TFTPコード以外での使用は想定していない。
 *
 * @param udpdev
 *      読み込むUDPデバイス
 * @param pkt
 *      読み込みに使用するバッファ
 * @param parent
 *      パケットを読み込んだ時に通知するスレッド
 *
 * @return
 *      このスレッドは復帰しない
 */
thread tftpRecvPackets(int udpdev, struct tftpPkt *pkt, tid_typ parent)
{
    int result;

    for (;;)
    {
        /* 親スレッドがこのスレッドに処理を指示するのを待つ */
        receive();

        /* UDPデバイスからパケットを読み込む（ブロックする） */
        result = read(udpdev, pkt, TFTP_MAX_PACKET_LEN);

        /* 読み込んだパケットの長さ（または、エラーが発生した場合はSYSEND）を親に送信する */
        send(parent, result);
    }

    /* このスレッドはkillされた。復帰しない */
    return SYSERR;
}
