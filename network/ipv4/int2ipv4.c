/**
 * @file int2ipv4.c
 */

#include <stddef.h>
#include <stdint.h>
#include <ipv4.h>
#include <network.h>

/**
 * @ingroup ipv4
 *
 * uint32_tのIPv4アドレスをnetaddrに変換する.
 *
 * @param addr 変換するuint32_tのIPv4アドレス文字列
 * @param ip 変換結果を収めるnetaddr構造体
 *
 * @return
 *      変換が成功したら ::OK; それ以外は ::SYSERR.
 */
syscall int2ipv4(const uint32_t addr, struct netaddr *ip)
{
    /* 1. 引数のエラーチェック */
    if (NULL == ip)
    {
        return SYSERR;
    }

    ip->addr[0] = (uint8_t) ((addr >> 24) & 0xff);
    ip->addr[1] = (uint8_t) ((addr >> 16) & 0xff);
    ip->addr[2] = (uint8_t) ((addr >> 8)  & 0xff);
    ip->addr[3] = (uint8_t) ((addr >> 0)  & 0xff);
    ip->type = NETADDR_IPv4;
    ip->len = IPv4_ADDR_LEN;
    return OK;
}

/**
 * @ingroup ipv4
 *
 * ネットアドレスをuint32_tに変換する.
 *
 * @param ip 変換するnetaddr構造体
 * @param addr 変換結果を格納するuint32_tへのポインタ
 *
 * @return
 *      変換が成功したら ::OK; それ以外は ::SYSERR.
 */
syscall ipv42int(const struct netaddr *ip, uint32_t *addr)
{
    /* 1. 引数のエラーチェック */
    if (NULL == ip || NULL == addr)
    {
        return SYSERR;
    }

    *addr = (((uint32_t)ip->addr[0]) << 24) | (((uint32_t)ip->addr[1]) << 16) |
            (((uint32_t)ip->addr[2]) << 8)  | (((uint32_t)ip->addr[3]) << 0);
    return OK;
}