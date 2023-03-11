/**
 * @file     udpRead.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <interrupt.h>
#include <string.h>
#include <udp.h>

/**
 * @ingroup udpexternal
 *
 * UDPデバイスから次のUDPパケットを読み込み、そのデータを
 * 提供されたバッファに配置する。
 *
 * デフォルトモードでは、結果のデータはUDPペイロードとなる。UDPが
 * @ref UDP_FLAG_PASSIVE "パッシブモード"の場合、結果のデータは
 * UDP疑似ヘッダとなり、直後にUDPヘッダが続き、さらに直後にUDP
 * ペイロードが続く。
 *
 * @param devptr
 *      UDPデバイス用のデバイステーブルエントリ.
 * @param buf
 *      読み込んだデータを置くバッファ.
 * @param len
 *      読み込むデータの最大量（ @p buf の長さ).
 *
 * @return
 *      成功の場合、読み込んだバイト数を返す。通常、これはUDPパケットの
 *      サイズである（上記のパッシブモードについての注意を参照）が、
 *      特別なケースとして、UDPがノンブロッキングモードでパケットが
 *      利用できない場合は0になり、実際に利用できたデータ量が @p len
 *      より大きい場合は @p lenになる。また、UDPデバイスがまだ開かれて
 *      いなかったり、パケットを読み込む際に閉じらていた場合は
 *      ::SYSERR が返される。
 */
devcall udpRead(device *devptr, void *buf, uint len)
{
    struct udp *udpptr;
    irqmask im;
    const struct udpPseudoHdr *pseudo;
    const struct udpPkt *udppkt;
    uint count;
    const void *data;

    udpptr = &udptab[devptr->minor];

    im = disable();

    /* UDPデバイスはオープンされていること  */
    if (UDP_OPEN != udpptr->state)
    {
        restore(im);
        return SYSERR;
    }

    /* UDPデバイスがノンブロッキングモードの場合、待機することなく
     * 少なくとも1つパケットが利用できることが必要である */
    if ((udpptr->flags & UDP_FLAG_NOBLOCK) && (udpptr->icount < 1))
    {
        restore(im);
        return 0;
    }

    /* UDPパケットが利用可能になるまで待機する */
    wait(udpptr->isem);

    /* パケットの待機中にUDPデバイスがクローズされないこと  */
    if (UDP_OPEN != udpptr->state)
    {
        restore(im);
        return SYSERR;
    }

    /* 循環バッファから次のUDPパケットを取得した後、それを削除する。
     * Get the next UDP packet from the circular buffer, then remove it.
     * 注意: 通常、これを行った後に割り込みを復元することは安全であるが、
     *  udpClose()との競合を避ける必要がある。 */
    pseudo = (const struct udpPseudoHdr *)udpptr->in[udpptr->istart];
    udpptr->istart = (udpptr->istart + 1) % UDP_MAX_PKTS;
    udpptr->icount--;

    /* UDPヘッダーへのポインタをセットする。これは疑似ヘッダーの直後である */
    udppkt = (const struct udpPkt *)(pseudo + 1);

    /* UDPデータを指定されたバッファにコピーする。ドキュメントのとおり、
     * コピーされる実際のデータはUDPデバイスの現在のモードによる。さらに、
     * 最大でも呼び出し元が要求したデータ量だけをコピーするよう注意する。 */
    if (UDP_FLAG_PASSIVE & udpptr->flags)
    {
        count = udppkt->len + sizeof(struct udpPseudoHdr);
        data = pseudo;
    }
    else
    {
        count = udppkt->len - UDP_HDR_LEN;
        data = udppkt->data;
    }
    if (count > len)
    {
        count = len;
    }
    memcpy(buf, data, count);

    /* パケットバッファを解放し、割り込みを復元し、読み込んだバイト数を
     * 返す。 */
    udpFreebuf((struct udpPkt *)pseudo);
    restore(im);
    return count;
}
