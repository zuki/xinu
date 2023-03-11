/**
 * @file     udpDemux.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <udp.h>

#define NO_MATCH      0
#define DEST_MATCH    1
#define PARTIAL_MATCH 2
#define FULL_MATCH    3

/**
 * @ingroup udpinternal
 *
 * UDPパケットのUDPソケットを探す
 * @param dstpt UDPパケットのあて先ポート
 * @param srcpt UDPパケットの送信元ポート
 * @param dstip UDPパケットのあて先IPアドレス
 * @param srcip UDPパケットの送信元IPアドレス
 * @return もっと完全に一致するソケット、一致するものがなかった場合はNULL
 * @pre-condition 割り込みはすでに無効になっている
 * @post-condition 割り込みは依然として無効である
 */
struct udp *udpDemux(ushort dstpt, ushort srcpt, const struct netaddr *dstip,
                     const struct netaddr *srcip)
{
    struct udp *udpptr = NULL;
    uint i;
    uint match = NO_MATCH;

    /* ベストマッチを見つけるためにすべてのUDPデバイスを走査 */
    for (i = 0; i < NUDP; i++)
    {
        if (udptab[i].state == UDP_FREE)
        {
            /* skip this entry */
            continue;
        }
        if (!netaddrequal(&udptab[i].localip, dstip))
        {
            /* エントリはあて先にバインドされていない */
            continue;
        }

        /* 完全に一致するものがベスト */
        if (match < FULL_MATCH
            && (udptab[i].localpt == dstpt)
            && (udptab[i].remotept == srcpt)
            && (netaddrequal(&udptab[i].remoteip, srcip)))
        {
            udpptr = &udptab[i];
            match = FULL_MATCH;
            /* 完全一致以上のベストはない */
            break;
        }

        /* 送信元とあて先のポートが一致する場合が2番目のマッチ */
        if (match < PARTIAL_MATCH
            && (udptab[i].localpt == dstpt)
            && (udptab[i].remotept == srcpt)
            && (udptab[i].remoteip.type == NULL))
        {
            udpptr = &udptab[i];
            match = PARTIAL_MATCH;
        }

        /* あて先のポートだけが一致する場合が最低限のマッチ */
        if (match < DEST_MATCH
            && (udptab[i].localpt == dstpt)
            && (udptab[i].remotept == NULL)
            && (udptab[i].remoteip.type == NULL))
        {
            udpptr = &udptab[i];
            match = DEST_MATCH;
        }
    }

    return udpptr;
}
