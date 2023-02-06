/**
 * @file dot2ipv4.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <ipv4.h>
#include <network.h>
#include <stdio.h>

/**
 * @ingroup ipv4
 *
 * ドット付き10進記法のIPv4アドレスをnetaddrに変換する.
 *
 * @param str
 *      変換するドット付き10進記法のIPv4アドレス文字列
 * @param ip
 *      変換結果を収めるnetaddr構造体
 *
 * @return
 *      変換が成功したら ::OK; それ以外は ::SYSERR.
 */
syscall dot2ipv4(const char *str, struct netaddr *ip)
{
    uint o0, o1, o2, o3;
    char tmp;

    /* ポインタのエラーチェック */
    if ((NULL == str) || (NULL == ip))
    {
        return SYSERR;
    }

    if (4 != sscanf(str, "%3u.%3u.%3u.%3u%c", &o0, &o1, &o2, &o3, &tmp) ||
        o0 > 0xff || o1 > 0xff || o2 > 0xff || o3 > 0xff)
    {
        return SYSERR;
    }

    ip->addr[0] = o0;
    ip->addr[1] = o1;
    ip->addr[2] = o2;
    ip->addr[3] = o3;
    ip->type = NETADDR_IPv4;
    ip->len = IPv4_ADDR_LEN;
    return OK;
}
