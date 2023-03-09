/**
 * @file dns.h
 */
/* SEE COPYRIGHT file */

#ifndef _DNS_H_
#define _DNS_H_

#include <stddef.h>
#include <stdint.h>
#include <ipv4.h>

//#define TRACE_DNS CONSOLE

#ifdef TRACE_DNS
#  include <stdio.h>
#  include <thread.h>
#  define DNS_TRACE(format, ...)                                     \
do                                                                    \
{                                                                     \
    fprintf(TRACE_DNS, "%s:%d (%d) ", __FILE__, __LINE__, gettid());     \
    fprintf(TRACE_DNS, format, ## __VA_ARGS__);                          \
    fprintf(TRACE_DNS, "\n");
} while (0)
#else
#  define DNS_TRACE(format, ...)
#endif

/* Constants and data structures used for DNS */
/** @ingroup dns
 * @def DNS_TIMEOUT
 * @brief 応答のタイムアウト(ms) */
#define DNS_TIMEOUT     3000
/** @ingroup dns
 * @def DNS_RETRY
 * @brief 再試行の回数 */
#define DNS_RETRY       3
/** @ingroup dns
 * @def DNS_PORT
 * @brief DNS UDPウェルノウンポート */
#define DNS_PORT        53
/** @ingroup dns
 * @def DNS_LOCAL_PORT
 * @brief ローカルUDPポート */
#define DNS_LOCAL_PORT  51525
/** @ingroup dns
 * @def DNS_DATASIZE
 * @brief データ領域のサイズ */
#define DNS_DATASIZE    500

/** @ingroup dns
 * DNS問い合わせ/応答パケット構造体
*/
struct    dnspkt {
    uint16_t    id;             /**< DNSクエリID             */
    struct    {
        uint8_t rd:1;           /**< 再起サービスを希望       */
        uint8_t tc:1;           /**< メーセージの切り詰めあり */
        uint8_t aa:1;           /**< 正式な回答回答           */
        uint8_t opcode:4;       /**< オペレーションコード     */
        uint8_t qr:1;           /**< 問い合わせ=0, 応答=1     */
    };
    struct    {
        uint8_t rcode:4;        /**< レスポンスコード          */
        uint8_t z:3;            /**< 予約, 0とすること         */
        uint8_t ra:1;           /**< 再起サービスが可能       */
    };
    uint16_t    qucount;        /**< 質問回数                */
    uint16_t    ancount;        /**< 回答のRR数              */
    uint16_t    nscount;        /**< ネームサーバのRR数      */
    uint16_t    arcount;        /**< 追加情報のRR数          */
    char    data[DNS_DATASIZE];   /**< DNSデータ領域        */
};

/** @ingroup dns
 * @def DNS_QT_A
 * @brief QType値: DNSアドレスタイプ (A) */
#define DNS_QT_A        1
/** @ingroup dns
 * @def DNS_QT_NS
 * @brief QType値: DNSネームサーバタイプ */
#define DNS_QT_NS       2
/** @ingroup dns
 * @def DNS_QT_CNAME
 * @brief QType値: CNAMEタイプ */
#define DNS_QT_CNAME    5

/* QClass values */
/** @ingroup dns
 * @def DNS_QC_IN
 * @brief QClass値: インターネット */
#define DNS_QC_IN       1

/** @ingroup dns
 * DNS質問構造体
 */
struct    dns_q {
    char        *qname;         /**< 問い合わせの対象ドメイン名 */
    uint16_t    *qtype;         /**< 問い合わせのタイプ         */
    uint16_t    *qclass;        /**< 問い合わせのクラス         */
};

/** @ingroup dns
 * DNSリソースレコード構造体
 */
struct    dns_rr {
    char        *rname;         /* Domain Name                  */
    uint16_t    *rtype;         /* Resource Record Type         */
    uint16_t    *rclass;        /* Resource Record Class        */
    uint32_t    *ttl;           /* Resource Record Time-to-Live */
    uint16_t    *rdlen;         /* Resource Record RD Length    */
    char        *rdata;         /* Resource Record Data area    */
};

int32_t dnsQuery(char *dname, char *data, uint16_t type);
syscall dnsGetA(char *dname, struct dnspkt *rpkt, uint32_t *addr);
syscall dnsGetCNAME(char *dname, struct dnspkt *rpkt, char *cname);

syscall dnsLookup(const struct netif *netptr, char *dname, struct netaddr *addr);
syscall dnsResolveA(const struct netif *netptr, char *dname, struct netaddr *addr);
syscall dnsResolveCNAME(const struct netif *netptr, char *dname, char *cname);

#endif
