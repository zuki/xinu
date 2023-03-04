/**
 * @file     netaddrhost.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <stddef.h>
#include <network.h>

/**
 * @ingroup network
 *
 * ネットワークアドレスのホスト部分を計算する。これは基本的には
 * netaddrmask()でのマスキングとは反対の演算である。たとえば、IPv4で
 * 192.168.0.50 からマスク 255.255.255.0 でホスト部分を取り出すと
 * 0.0.0.50 になる。
 *
 * @param a
 *      ホスト部分を取り出すネットワークアドレス
 * @param mask
 *      使用するネットマスク
 *
 * @return
 *      @p a と @p mask が同じ種別のネットワークアドレスでない場合は SYSERR;
 *      そうでなければ OK を返し、@a にはネットワークアドレスのホスト部分だけの
 *      値に変更する
 */
syscall netaddrhost(struct netaddr *a, const struct netaddr *mask)
{
    uint i;

    if (a->type != mask->type || a->len != mask->len)
    {
        return SYSERR;
    }

    for (i = 0; i < a->len; i++)
    {
        a->addr[i] &= ~mask->addr[i];
    }

    return OK;
}
