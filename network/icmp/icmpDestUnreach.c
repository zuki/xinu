/**
 * @file icmpDestUnreach.c
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <ipv4.h>
#include <icmp.h>
#include <string.h>

/**
 * @ingroup icmp
 *
 * ICMP宛先到達不可メッセージを作成する.
 * @param unreached 送信できなかったパケット
 * @param code      ICMP宛先到達不可コード番号
 * @return パケットが送信されたら OK; それ以外は SYSERR
 */
syscall icmpDestUnreach(const struct packet *unreached, uchar code)
{
    struct packet *pkt;
    const struct ipv4Pkt *ip;
    struct netaddr dst;
    int result;
    int ihl;
    struct netaddr src;

    ICMP_TRACE("destination unreachable (%d)", code);
    pkt = netGetbuf();
    if (SYSERR == (int)pkt)
    {
        ICMP_TRACE("Failed to acquire packet buffer");
        return SYSERR;
    }

    ip = (const struct ipv4Pkt *)unreached->nethdr;
    dst.type = NETADDR_IPv4;
    dst.len = IPv4_ADDR_LEN;
    /* Send error message back to original source.                */
    memcpy(dst.addr, ip->src, dst.len);
    ihl = (ip->ver_ihl & IPv4_IHL) * 4;

    /* Message will contain at least ICMP_DEF_DATALEN             */
    /*  of packet in question, as per RFC 792.                    */
    pkt->len = ihl + ICMP_DEF_DATALEN;
    pkt->curr -= pkt->len;

    memcpy(pkt->curr, ip, ihl + ICMP_DEF_DATALEN);
    /* ペイロードの最初の4オクテットは使用しない */
    pkt->curr -= 4;
    pkt->len += 4;
    *((ulong *)pkt->curr) = 0;

    src.type = 0;

    result = icmpSend(pkt, ICMP_UNREACH, code, pkt->len, &src, &dst);

    netFreebuf(pkt);
    return result;
}
