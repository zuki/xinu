/**
 * @file dns.c
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <thread.h>
#include <device.h>
#include <network.h>
#include <ipv4.h>
#include <dns.h>
#include <udp.h>

#define NULLCH '\0'

/**
 * @ingroup dns
 * ドメイン名を検索して、IPアドレスを取得する.
 *
 * @param[in] netptr ネットワークインタフェースへのポインタ.
 * @param[in] dname 解決するドメイン名
 * @param[out] addr 解決したアドレスを収めるnetaddr構造体へのポインタ
 * @return 成功したらOK; そうでなければ SYSERR
*/
syscall dnsLookup(const struct netif *netptr, char *dname, struct netaddr *addr)
{
    char           *p;          /* Pointer to walk the name                 */
    char            ch;         /* One character in the name                */
    uint8_t         dotted;     /* Is sname dotted decimal?                 */
    char cname[1024];           /* CNAME */

    /* 1.1. 引数のエラーチェック1 */
    if (dname == NULL || addr == NULL) {
        DNS_TRACE("Bad parameter: dname %s, addr 0x%p.", dname, addr);
        return SYSERR;
    }

    /* 1.2. 引数のエラーチェック2  */
#if NETHER
    if (netptr == NULL)
#endif
    {
        DNS_TRACE("Network is down.");
        return SYSERR;
    }

#ifdef TRACE_DNS
    char str_ip[20], str_dns[20];
    netaddrsprintf(str_ip, &netptr->ip);
    netaddrsprintf(str_dns, &netptr->dns);
    DNS_TRACE("ip %s dns %s\n", str_ip, str_dns);
#endif

    /* 2. dnameがドット付き10進値の場合はipv4に変換して返す */
    p = dname;
    ch = *p++;
    dotted = TRUE;
    while (ch != NULLCH) {
        if ( (ch != '.') && ( (ch < '0') || (ch > '9') ) ) {
            dotted = FALSE;
            break;
        }
        ch = *p++;
    }
    if (dotted) {
        /* dnameは数字とドットしか含まれていない */
        return dot2ipv4(dname, addr);
    }

    /* 3. dname == 'localhost'の場合は"127.0.0.1"を返す */
    if (strncmp(dname, "localhost", 9) == 0 ) {
        /* make localhost into 127.0.0.1 */
        return dot2ipv4("127.0.0.1", addr);
    }

    /* 3. A レコードを取得 */
    if (OK == dnsResolveA(netptr, dname, addr)) {
        /* 3.1 IPv4アドレスに変換して返す */
        return OK;
    }

    /* 4. CNAME レコードを取得 */
    if (SYSERR == dnsResolveCNAME(netptr, dname, cname)) {
        return SYSERR;
    }

    /* 5. CNAMEでA レコードを取得 */
    return dnsResolveA(netptr, cname, addr);
}
