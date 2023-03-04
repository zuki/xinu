/**
 * @file     netaddrequal.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <network.h>
#include <string.h>

/**
 * @ingroup network
 *
 * 2つのネットワークアドレスを比較する.
 * @param a 第1ネットワークアドレス
 * @param b 第2ネットワークアドレス
 * @return アドレスが等しい場合は ::TRUE, そうでなければ ::FALSE.
 */
bool netaddrequal(const struct netaddr *a, const struct netaddr *b)
{
    return (a->type == b->type &&
            a->len == b->len &&
            0 == memcmp(a->addr, b->addr, a->len));
}
