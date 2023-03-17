/**
 * @file ntpClient.c
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <thread.h>
#include <device.h>
#include <network.h>
#include <ipv4.h>
#include <udp.h>
#include <dns.h>
#include <ntp.h>

/**
 * @ingroup ntp
 * NTPサーバから現在時刻を取得する.
 *
 * @param[in] netptr ネットワークインタフェースへのポインタ.
 * @param[out] epoc epoc秒数
 * @return 成功したらOK; そうでなければ SYSERR
*/
syscall ntpGetEpoc(const struct netif *netptr, uint32_t *epoc)
{
    char           *p;
    char            ch;
    uint8_t         dotted = TRUE;
    char            server[128];
    struct netaddr  addr;
    struct ntpPkt   sntp, rntp;
    uint16_t        udpdev;
    int             i, plen = sizeof(struct ntpPkt);

    /* 1. 引数のエラーチェック2  */
#if NETHER
    if (netptr == NULL)
#endif
    {
        NTP_TRACE("Network is down.");
        return SYSERR;
    }

#ifdef TRACE_NTP
    char str_ip[20], str_dns[20];
    netaddrsprintf(str_ip, &netptr->ip);
    netaddrsprintf(str_dns, &netptr->dns);
    NTP_TRACE("ip %s dns %s\n", str_ip, str_dns);
#endif

    /* 2. serverがドット付き10進値の場合はipv4に変換して返す */
    memcpy(server, NTP_SERVER, strlen(NTP_SERVER)+1);
    p = server;
    ch = *p++;
    while (ch != NULLCH) {
        if ( (ch != '.') && ( (ch < '0') || (ch > '9') ) ) {
            dotted = FALSE;
            break;
        }
        ch = *p++;
    }
    if (dotted) {
        /* dnameは数字とドットしか含まれていない */
        if (dot2ipv4(server, &addr) == SYSERR) {
            return SYSERR;
        }
    } else {
        if (dnsLookup(netptr, server, &addr) == SYSERR) {
            return SYSERR;
        }
    }

    /* 3. udpデバイスを取得する */
    udpdev = udpAlloc();
    if (SYSERR == udpdev) {
        DNS_TRACE("Failed to allocate first UDP device.");
        return SYSERR;
    }

    /* 4. udpデバイスをopen */
    if (SYSERR == open(udpdev, &netptr->ip, &addr, UDP_PORT_NTP, UDP_PORT_NTP)) {
        DNS_TRACE("Failed to open NTP device.");
        udptab[udpdev - UDP0].state = UDP_FREE;
        return SYSERR;
    }

    /* 5. NTPパケットを作成 */
    memset((char *)&sntp, 0, plen);
    sntp.mode = NTP_MODE_CLIENT;
    sntp.vn = NTP_VN_4;
    sntp.li = NTP_LI_UNKNOWN;
    sntp.stratum = 0;
    sntp.poll = 10;
    sntp.precision = 0;
    memcpy((char *)&sntp.ref_id, "XCIR", 4);

    /* 6. 規定の回数繰り返し問い合わせを送信し、応答を待機する */
    for (i = 0 ; i < NTP_RETRY ; i++) {
        /* 6.1 問い合わせメッセージを送信する */
        if (plen != write(udpdev, &sntp, plen)) {
            DNS_TRACE("Error sending NTP packet");
            continue;
        }
        /* 6.2 応答を読み込む */
        if (read(udpdev, (char *)&rntp, plen) == SYSERR) {
            DNS_TRACE("Error receiving NTP packet");
            continue;
        }
        /* 6.3 現在時刻を抽出する */
        if (rntp.tx_ts.seconds != 0) {
            /* 7.4 抽出に成功 */
            *epoc = net2hl(rntp.tx_ts.seconds);
            break;
        }

    }
    /* 8. udpデバイスをクローズ */
    close(udpdev);

    /* 10. 試行回数をオーバー（アドレスが得られず） */
    if (i >= NTP_RETRY) {
        NTP_TRACE("Retry over.");
        return SYSERR;
    }

    return OK;
}
