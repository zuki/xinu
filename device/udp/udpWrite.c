/**
 * @file     udpWrite.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <udp.h>

/**
 * @ingroup udpexternal
 *
 * UDPデバイスにデータを書き込むことにより、UDPデバイスを構成した
 * アドレス/ポートを使用して、1つ以上のUDPパケットとしてネットワーク上に
 * 送信する。
 *
 * 注意: ネットワークスタックの下位レベルによっては、この関数は
 * UDPパケットをバッファするだけで、送信は後で行われる可能性がある。
 * したがって、この関数が復帰咲いた際は実際にはUDPパケットはまだ
 * 送信されていない可能性がある。
 *
 * UDPデバイスはオープンされており、この関数の実行中もオープンしている
 * 必要がある。
 *
 * @param devptr
 *      UDPデバイス用のデバイスエントリ
 * @param buf
 *      送信するデータを収めたバッファ。UDPデバイスがデフォルト
 *      モードの場合、これは送信すべきUDPペイロードであると解釈され、
 *      そのサイズが ::UDP_MAX_DATALEN バイトを超える場合は複数の
 *      UDPパッケージに分割される。一方、UDPデバイスが @ref
 *      UDP_FLAG_PASSIVE "パッシブモード"の場合、データはUDP疑似
 *      ヘッダーを含む1つのUDPパケットであり、直後にUDPヘッダーと
 *      UDPペイロードが続くものと解釈される。
 * @param len
 *      送信するデータのバイト数（@p buf の長さ).
 *
 * @return
 *      いずれかのパケットの送信に成功した場合は、送信に成功した
 *      データ数を返す。エラーの場合、これは @p len より小さい場合が
 *      ある。UDPデバイスがパッシブモードではなく、 @p lenが0であった
 *      場合は、パケットは送信されず、0が返される。それ以外は ::SYSERR
 *      を返す。
 */
devcall udpWrite(device *devptr, const void *buf, uint len)
{
    struct udp *udpptr;
    int result;

    udpptr = &udptab[devptr->minor];

    if (udpptr->flags & UDP_FLAG_PASSIVE)
    {
        /* パッシブUDPデバイスの場合、pseudoheader + header + payload
         * が渡される */
        if (len > sizeof(struct udpPseudoHdr) + UDP_HDR_LEN + UDP_MAX_DATALEN ||
            len < sizeof(struct udpPseudoHdr) + UDP_HDR_LEN)
        {
            UDP_TRACE("Invalid size (%d) passed to passive UDP device", len);
            return SYSERR;
        }
        result = udpSend(udpptr, len, buf);
        if (OK == result)
        {
            return len;
        }
        else
        {
            return SYSERR;
        }
    }
    else
    {
        uint pktsize;
        uint count;

        /* リモートのポートとipが指定されているかチェックする */
        if (0 == udpptr->remotept || 0 == udpptr->remoteip.type)
        {
            UDP_TRACE("No specified remote port or IP address.");
            return SYSERR;
        }

        /* 実際の書き出しを実行する */
        for (count = 0; count < len; count += pktsize)
        {
            uint bytes_remaining = len - count;

            if (bytes_remaining >= UDP_MAX_DATALEN)
            {
                pktsize = UDP_MAX_DATALEN;
            }
            else
            {
                pktsize = bytes_remaining;
            }

            result = udpSend(udpptr, pktsize, (const uchar*)buf + count);
            if (OK != result)
            {
                if (0 == count)
                {
                    return SYSERR;
                }
                else
                {
                    return count;
                }
            }
        }
        return len;
    }
}
