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
struct    dnsPkt {
    uint16_t    id;             /**< DNSクエリID             */
    struct    {
        uint8_t rd:1;           /**< 再帰サービスを希望       */
        uint8_t tc:1;           /**< メーセージの切り詰めあり */
        uint8_t aa:1;           /**< 正式な回答回答           */
        uint8_t opcode:4;       /**< オペレーションコード     */
        uint8_t qr:1;           /**< 問い合わせ=0, 応答=1     */
    };
    struct    {
        uint8_t rcode:4;        /**< レスポンスコード          */
        uint8_t z:3;            /**< 予約, 0とすること         */
        uint8_t ra:1;           /**< 再帰サービスが可能       */
    };
    uint16_t    qucount;        /**< 質問回数                */
    uint16_t    ancount;        /**< 回答のRR数              */
    uint16_t    nscount;        /**< ネームサーバのRR数      */
    uint16_t    arcount;        /**< 追加情報のRR数          */
    char    data[DNS_DATASIZE];   /**< DNSデータ領域        */
};

/** @ingroup dns
 * @def DNS_Q
 * @brief 問い合わせ */
#define DNS_Q       0
/** @ingroup dns
 * @def DNS_R
 * @brief 応答 */
#define DNS_R       1

/** @ingroup dns
 * @def DNS_CODE_Q
 * @brief OPCODE: 順引き */
#define DNS_CODE_Q          0
/** @ingroup dns
 * @def DNS_CODE_IQ
 * @brief OPCODE: 逆引き */
#define DNS_CODE_IQ         1
/** @ingroup dns
 * @def DNS_CODE_STATUS
 * @brief OPCODE: サーバステータス */
#define DNS_CODE_STATUS     2

/** @ingroup dns
 * @def DNS_QT_A
 * @brief QType値: DNSアドレス (A) */
#define DNS_QT_A        1
/** @ingroup dns
 * @def DNS_QT_NS
 * @brief QType値: DNSネームサーバ */
#define DNS_QT_NS       2
/** @ingroup dns
 * @def DNS_QT_CNAME
 * @brief QType値: CNAME */
#define DNS_QT_CNAME    5
/** @ingroup dns
 * @def DNS_QT_SOA
 * @brief QType値: CNAME */
#define DNS_QT_SOA      6
/** @ingroup dns
 * @def DNS_QT_PTR
 * @brief QType値: PTR */
#define DNS_QT_PTR      12
/** @ingroup dns
 * @def DNS_QT_HINFO
 * @brief QType値: HINFO */
#define DNS_QT_HINFO    13
/** @ingroup dns
 * @def DNS_QT_MX
 * @brief QType値: MX */
#define DNS_QT_MX       15
/** @ingroup dns
 * @def DNS_QT_TXT
 * @brief QType値: TXT */
#define DNS_QT_TXT      16
/** @ingroup dns
 * @def DNS_QT_AXFR
 * @brief QType値: AXFR */
#define DNS_QT_AXFR     252
/** @ingroup dns
 * @def DNS_QT_ALL
 * @brief QType値: ALL */
#define DNS_QT_ALL      255

/* QClass values */
/** @ingroup dns
 * @def DNS_QC_IN
 * @brief QClass値: インターネット */
#define DNS_QC_IN       1
/** @ingroup dns
 * @def DNS_QC_ANY
 * @brief QClass値: ANY */
#define DNS_QC_ANY      255

/** @ingroup dns
 * @def DNS_RCODE_NOERROR
 * @brief RCode値: NOERROR */
#define DNS_RCODE_NOERROR   0
/** @ingroup dns
 * @def DNS_RCODE_FORMERR
 * @brief RCode値: FORMERR */
#define DNS_RCODE_FORMERR   1
/** @ingroup dns
 * @def DNS_RCODE_SERVFAIL
 * @brief RCode値: SERVFAIL */
#define DNS_RCODE_SERVFAIL  2
/** @ingroup dns
 * @def DNS_RCODE_NXDOMAIN
 * @brief RCode値: NXDOMAIN  */
#define DNS_RCODE_NXDOMAIN  3
/** @ingroup dns
 * @def DNS_RCODE_NOTIMP
 * @brief RCode値: NOTIMP */
#define DNS_RCODE_NOTIMP    4
/** @ingroup dns
 * @def DNS_RCODE_REFUSED
 * @brief RCode値: REFUSED */
#define DNS_RCODE_REFUSED   5

/** @ingroup dns
 * DNS質問構造体
 */
struct    dns_q {
    //char        *qname;         /**< 問い合わせの対象ドメイン名 */
    uint16_t    qtype;         /**< 問い合わせのタイプ         */
    uint16_t    qclass;        /**< 問い合わせのクラス         */
} __packed;

/** @ingroup dns
 * DNSリソースレコード構造体
 */
struct    dns_rr {
    //char        *rname;         /* Domain Name                  */
    uint16_t    rtype;         /* Resource Record Type         */
    uint16_t    rclass;        /* Resource Record Class        */
    uint32_t    ttl;           /* Resource Record Time-to-Live */
    uint16_t    rdlen;         /* Resource Record RD Length    */
    //char       *rdata;         /* Resource Record Data area    */
} __packed;

int32_t dnsQuery(char *dname, char *data, uint16_t type);
syscall dnsGetA(char *dname, struct dnsPkt *rpkt, uint32_t *addr);
syscall dnsGetCNAME(char *dname, struct dnsPkt *rpkt, char *cname);

syscall dnsLookup(const struct netif *netptr, char *dname, struct netaddr *addr);
syscall dnsResolveA(const struct netif *netptr, char *dname, struct netaddr *addr);
syscall dnsResolveCNAME(const struct netif *netptr, char *dname, char *cname);
uint32_t dnsGetRName(char *sop, char *son, char *dst);

#endif
