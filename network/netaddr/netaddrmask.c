/**
 * @file     netaddrmask.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <stddef.h>
#include <network.h>
#include <ipv4.h>

/** @ingroup network
 * グローバルIPブロードキャストアドレス
*/
const struct netaddr NETADDR_GLOBAL_IP_BRC = { NETADDR_IPv4, IPv4_ADDR_LEN,
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
};

/** @ingroup network
 * グローバルEthernetブロードキャストアドレス
*/
const struct netaddr NETADDR_GLOBAL_ETH_BRC = { NETADDR_ETHERNET, ETH_ADDR_LEN,
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
};

/**
 * @ingroup network
 *
 * ネットワークアドレスにマスクを適用する. たとえば、IPv4の
 * 192.168.0.50 を 255.255.255.0 でマスクすると 192.168.0.0 となる。
 *
 * @param a
 *      マスクするネットワークアドレス
 * @param mask
 *      適用するマスク
 *
 * @return
 *      @p a と @p mask が同じ種別のネットワークアドレスでない場合は SYSERR;
 *      そうでなければ OK を返し、@a にはマスクした値に変更する
 */
syscall netaddrmask(struct netaddr *a, const struct netaddr *mask)
{
    uint i;

    if (a->type != mask->type || a->len != mask->len)
    {
        return SYSERR;
    }

    for (i = 0; i < a->len; i++)
    {
        a->addr[i] &= mask->addr[i];
    }

    return OK;
}
