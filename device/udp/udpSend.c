/**
 * @file     udpSend.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <ipv4.h>
#include <network.h>
#include <string.h>
#include <udp.h>

/**
 * @ingroup udpinternal
 *
 * UDPデバイスを通じてUDPパケットを送信することにより、UDPデバイスを
 * 構成するアドレスとポートでネットワークに送信する。
 *
 * この関数はUDPコード内での使用を想定しており、外部コードからは
 * udpWrite()を呼び出すwrite()を使用すること。
 *
 * 注意: ネットワークスタックの下位レベルによっては、この関数はパケットを
 * バッファするだけで、送信は後で行われる場合がある。したがって、この関数が
 * 復帰しても実際にはパケットが送信されていない場合もある。
 *
 * UDPデバイスはオープンされており、この関数の実行中もオープンしている
 * 必要がある。
 *
 * @param udpptr
 *      UDPデバイス用の制御ブロックへのポインタ
 * @param datalen
 *      送信するデータのバイト数。現在のUDPデバイスモードにおいて
 *      正しいデータでなければならない（たとえば、パッシブモードの場合、
 *      UDP疑似ヘッダー長+UDPヘッダー長以上でなければならない）
 * @param buf
 *      送信するデータを収めたバッファ。デフォルトモードではこれは
 *      UDPペイロードであると解釈されるが、UDPデバイスがパッシブモードの
 *      場合はUDP疑似ヘッダにUDPヘッダーとUDPペイロードが続いたものと
 *      解釈される。
 *
 * @return パケットの送信に成功した場合は OK; それ以外は SYSERR、または、
 *      ipv4Send()が返したエラーコード
 */
syscall udpSend(struct udp *udpptr, ushort datalen, const void *buf)
{
    struct packet *pkt;
    struct udpPkt *udppkt;
    struct netaddr localip, remoteip;
    int result;

    pkt = netGetbuf();
    if (SYSERR == (int)pkt)
    {
        UDP_TRACE("Failed to allocate buffer");
        return SYSERR;
    }

    netaddrcpy(&localip, &(udpptr->localip));
    netaddrcpy(&remoteip, &(udpptr->remoteip));

    if (udpptr->flags & UDP_FLAG_PASSIVE)
    {
        const struct udpPseudoHdr *pseudo;

        pseudo = buf;
        datalen -= sizeof(struct udpPseudoHdr);
        /* パケット長をセットし、カレントポインタをその長さ分だけ
         * 戻す */
        pkt->len = datalen;
        /* ワードアライメントを保持するためにデータ長を丸める */
        pkt->curr -= (3 + (ulong)(pkt->len)) & ~0x03;

        /* パケットとヘッダーをコピーする */
        memcpy(pkt->curr, (pseudo + 1), datalen);
        udppkt = (struct udpPkt *)(pkt->curr);
        /* ネットワークオーダにフィールドを変換する */
        udppkt->srcPort = hs2net(udppkt->srcPort);
        udppkt->dstPort = hs2net(udppkt->dstPort);
        udppkt->len = hs2net(pkt->len);
        udppkt->chksum = 0;

        remoteip.len = localip.len;
        remoteip.type = localip.type;
        memcpy(&remoteip.addr, pseudo->dstIp, remoteip.len);

        if (0 != memcmp(&localip.addr, pseudo->srcIp, localip.len))
        {
            UDP_TRACE("Src IP does not match UDP passive IP.");
            netFreebuf(pkt);
            return SYSERR;
        }
    }
    else
    {
        datalen += UDP_HDR_LEN;
        /* pカット長をセットし、カレントポインタをその長さ分だけ
         * 戻す */
        pkt->len = datalen;
        /* ワードアライメントを保持するためにデータ長を丸める */
        pkt->curr -= (3 + (ulong)(pkt->len)) & ~0x03;

        /* UDPヘッダフィールドをセットし、パケットをデータで埋める */
        udppkt = (struct udpPkt *)(pkt->curr);
        udppkt->srcPort = hs2net(udpptr->localpt);
        udppkt->dstPort = hs2net(udpptr->remotept);
        udppkt->len = hs2net(pkt->len);
        udppkt->chksum = 0;

        memcpy(udppkt->data, buf, datalen - UDP_HDR_LEN);
    }

    /* UDPチェックサムを計算する（たまたまTCPの計算と同じである） */
    udppkt->chksum = udpChksum(pkt, datalen, &localip, &remoteip);

    /* UDPパケットをIPを通じて送信する */
    result = ipv4Send(pkt, &localip, &remoteip, IPv4_PROTO_UDP);

    if (SYSERR == netFreebuf(pkt))
    {
        UDP_TRACE("Failed to free buffer");
        return SYSERR;
    }

    return result;
}
