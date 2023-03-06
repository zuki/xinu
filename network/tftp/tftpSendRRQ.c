/**
 * @file tftpSendRRQ.c
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <tftp.h>
#include <device.h>
#include <string.h>

/**
 * @ingroup tftp
 *
 * TFTP RRQ（Read Request）パケットをUDP接続経由でTFTPサーバに送信する.
 * これはTFTPサーバに指定されたファイルの内容の送信を開始するよう指示する。
 * TFTPコード以外での使用は想定していない。
 *
 * @param udpdev
 *      UDPデバイスをオープンするためのデバイスディスクリプタ
 * @param filename
 *      要求するファイルの名前
 *
 * @return
 *      パケットの送信に成功したら OK; そうでなければ SYSERR
 */
syscall tftpSendRRQ(int udpdev, const char *filename)
{
    char *p;
    uint filenamelen;
    uint pktlen;
    struct tftpPkt pkt;

    /* ファイル名のサニティチェック  */
    filenamelen = strnlen(filename, 256);
    if (0 == filenamelen || 256 == filenamelen)
    {
        TFTP_TRACE("Filename is invalid.");
        return SYSERR;
    }

    TFTP_TRACE("RRQ \"%s\" (mode: octet)", filename);

    /* TFTP opcodeにRRQ（読み込み要求）をセット  */
    pkt.opcode = hs2net(TFTP_OPCODE_RRQ);

    /* ファイル名とモードをセット */
    p = pkt.RRQ.filename_and_mode;
    memcpy(p, filename, filenamelen + 1);
    p += filenamelen + 1;
    memcpy(p, "octet", 6);
    p += 6;

    /* 作成したパケットをUDPデバイスに書き込む */
    pktlen = p - (char*)&pkt;
    if (pktlen != write(udpdev, &pkt, pktlen))
    {
        TFTP_TRACE("Error sending RRQ");
        return SYSERR;
    }
    return OK;
}
