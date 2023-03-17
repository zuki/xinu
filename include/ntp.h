/**
 * @file ntp.h
*/

#ifndef _NTP_H
#define _NTP_H

#include <stddef.h>
#include <stdint.h>
#include <network.h>

//#define TRACE_NTP CONSOLE

#ifdef TRACE_NTP
#  include <stdio.h>
#  include <thread.h>
#  define NTP_TRACE(format, ...)                                     \
do                                                                    \
{                                                                     \
    fprintf(TRACE_NTP, "%s:%d (%d) ", __FILE__, __LINE__, gettid());     \
    fprintf(TRACE_NTP, format, ## __VA_ARGS__);                          \
    fprintf(TRACE_NTP, "\n");                                           \
} while (0)
#else
#  define NTP_TRACE(format, ...)
#endif

/** @ingroup ntp
 * @def NTP_SERVER
 * @brief NTPサーバ名 */
#define NTP_SERVER      "ntp.nict.jp"

/** @ingroup ntp
 * @def NTP_RETRY
 * @brief 再試行の回数 */
#define NTP_RETRY       3

/** @ingroup ntp
 * @def NTP_LI_UNKNOWN
 * @brief NTP Leap Indicator: 不明 */
#define NTP_LI_UNKNOWN  3

/** @ingroup ntp
 * @def NTP_VN_4
 * @brief NTPバージョン4 */
#define NTP_VN_4        4

/** @ingroup ntp
 * @def NTP_MODE_CLIENT
 * @brief NTPアソシエーションモード: クライアント */
#define NTP_MODE_CLIENT 3

/** @ingroup ntp
 * @struct ntpTS
 * @brief NTPタイムスタンプ構造体
 */
struct ntpTS {
    uint32_t seconds;       /**< epoc以来の秒数 */
    uint32_t picosecs;      /**< epoc以来の秒数未満 */
};

/** @ingroup ntp
 * @struct ntpPkt
 * @brief NTPパケット構造体
 */
struct    ntpPkt {
    struct    {
        uint8_t mode:3;         /**< アソシエーションのモード   */
        uint8_t vn:3;           /**< バージョン番号             */
        uint8_t li:2;           /**< Leap Indicator             */
    };
    uint8_t stratum;            /**< Stratum                    */
    uint8_t poll;               /**< NTPパケット送出間隔のlog2  */
    uint8_t precision;          /**< 精度のlog2                 */
    uint32_t rt_delay;          /**< PrimaryサーバとのRT遅延    */
    uint32_t rt_dispersion;     /**< Primaryサーバとの通信ゆらぎ*/
    uint32_t ref_id;            /**< 参照ID                     */
    struct ntpTS ref_ts;        /**< 最後に動悸した時刻         */
    struct ntpTS org_ts;        /**< 前回相手のNTPパケットの送信時刻*/
    struct ntpTS rx_ts;         /**< NTPパケットの前回受信時刻  */
    struct ntpTS tx_ts;         /**< NTPパケットの今回送信時刻  */
};

syscall ntpGetEpoc(const struct netif *netptr, uint32_t *epoc);

#endif
