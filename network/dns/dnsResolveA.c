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

/**
 * @ingroup dns
 * ドメイン名を検索して、IPアドレスを取得する.
 *
 * @param[in] netptr ネットワークインタフェースへのポインタ.
 * @param[in] dname 解決するドメイン名
 * @param[out] addr 解決したアドレスを収めるnetaddr構造体へのポインタ
 * @return 成功したらOK; そうでなければ SYSERR
*/
syscall dnsResolveA(const struct netif *netptr, char *dname, struct netaddr *addr)
{
    struct dnspkt   qpkt;       /* DNS問い合わせパケットバッファ            */
    struct dnspkt   rpkt;       /* DNS応答パケットバッファ                  */
    int32_t         qlen;       /* DNS問い合わせメッセージ長                */
    uint32_t        ipaddr;     /* 応答から抽出したIPアドレス(net2hl済み)   */
    int32_t         i;          /* Loop index                               */
    uint16_t        udpdev;     /* UDPデバイス番号                          */

    /* 1. 引数のエラーチェック2  */
#if NETHER
    if (netptr == NULL)
#endif
    {
        DNS_TRACE("Network is down.");
        return SYSERR;
    }

    /* 4. udpデバイスを取得する */
    udpdev = udpAlloc();
    if (SYSERR == udpdev) {
        DNS_TRACE("Failed to allocate first UDP device.");
        return SYSERR;
    }

    /* 5. udpデバイスをopen */
    if (SYSERR == open(udpdev, &netptr->ip, &netptr->dns, DNS_LOCAL_PORT, DNS_PORT)) {
        DNS_TRACE("Failed to open UDP device.");
        udptab[udpdev - UDP0].state = UDP_FREE;
        return SYSERR;
    }

    /* 6. DNS質問メッセージを作成 */
    memset((char *)&qpkt, 0, sizeof(struct dnspkt));
    qpkt.id = (uint16_t)gettid();
    qpkt.rd = 1;
    qpkt.qucount = hs2net(1);

    qlen = dnsQuery(dname, qpkt.data, DNS_QT_A);
    if ((uint16_t)qlen == SYSERR) {
        DNS_TRACE("Could not make a qeury messesage for A.");
        close(udpdev);
        return SYSERR;
    }

    /* 7. 規定の回数繰り返し問い合わせを送信し、応答を待機する */
    for (i = 0 ; i < DNS_RETRY ; i++) {
        /* 7.1 問い合わせメッセージを送信する */
        if (qlen != write(udpdev, &qpkt, qlen)) {
            DNS_TRACE("Error sending DNS Query Message");
            continue;
        }
        /* 7.2 応答を読み込む */
        if (read(udpdev, (char *)&rpkt, sizeof(struct dnspkt)) == SYSERR) {
            DNS_TRACE("Error receiving DNS Query Message");
            continue;
        }
        /* 7.3 アドレスを抽出する */
        if (dnsGetA(dname, &rpkt, &ipaddr) == SYSERR) {
            DNS_TRACE("Failed to get A.");
            continue;
        }
        /* 7.4 抽出に成功 */
        break;
    }
    /* 8. udpデバイスをクローズ */
    close(udpdev);

    /* 10. 試行回数をオーバー（アドレスが得られず） */
    if (i >= DNS_RETRY) {
        DNS_TRACE("Retry over.");
        return SYSERR;
    }

    /* 11. IPv4アドレスに変換 */
    return int2ipv4(ipaddr, addr);
}
