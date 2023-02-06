/**
 * @file ipv4RecvValid.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <ipv4.h>

/**
 * @ingroup ipv4
 *
 * IPヘッダが有効か確認し、必要なbad paramメッセージを送信する.
 * @param ip 確認するIPパケット
 * @return パケットが有効の場合 TRUE; そうでなければ FALSE
 */
bool ipv4RecvValid(struct ipv4Pkt *ip)
{
	int ip_ihl = ip->ver_ihl & IPv4_IHL;

    /* バージョンをチェックする */
    if (((ip->ver_ihl & IPv4_VER) >> 4) != IPv4_VERSION)
    {
        return FALSE;
    }

    /* ヘッダ長をチェックする */
    if ((ip_ihl < IPv4_MIN_IHL) || (ip_ihl > IPv4_MAX_IHL))
    {
        return FALSE;
    }

    /* 合計サイズをチェックする*/
    if (ip->len < (IPv4_MIN_IHL * 4))
    {
        return FALSE;
    }

    /* チェックサムを確認する */
    if (netChksum(ip, (ip_ihl * 4)) != 0)
    {
        return FALSE;
    }

    return TRUE;
}
