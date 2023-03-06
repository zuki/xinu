/**
 * @file tftpSendACK.c
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <tftp.h>
#include <device.h>

/**
 * @ingroup tftp
 *
 * TFTP ACK（Acknowledge）パケットをUDP接続経由でTFTPサーバーに送信する.
 * TFTP Get転送の場合、これは指定したデータパケット（特定のブロック番号を
 * 持つ）を受信したことをTFTPサーバーに通知する。TFTPコード以外での使用は
 * 意図していない。
 *
 * @param udpdev
 *      UDPデバイスをオープンするためのデバイスディスクリプタ
 * @param block_number
 *      ackするブロック番号
 *
 * @return
 *      パケットの送信に成功したら OK; そうでなければ SYSERR
 */
syscall tftpSendACK(int udpdev, ushort block_number)
{
    struct tftpPkt pkt;

    TFTP_TRACE("ACK block %u", block_number);
    pkt.opcode = hs2net(TFTP_OPCODE_ACK);
    pkt.ACK.block_number = hs2net(block_number);
    if (4 != write(udpdev, &pkt, 4))
    {
        TFTP_TRACE("Error sending ACK");
        return SYSERR;
    }
    return OK;
}
