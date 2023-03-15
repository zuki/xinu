/**
 * @file snoop.h
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _SNOOP_H_
#define _SNOOP_H_

#include <stddef.h>
#include <arp.h>
#include <ethernet.h>
#include <ipv4.h>
#include <mailbox.h>
#include <network.h>
#include <tcp.h>
#include <udp.h>
#include <dhcp.h>

/* Tracing macros */
//#define TRACE_SNOOP     TTY1
#ifdef TRACE_SNOOP
#include <stdio.h>
#define SNOOP_TRACE(...)     { \
	fprintf(TRACE_SNOOP, "%s:%d (%d) ", __FILE__, __LINE__, gettid()); \
    fprintf(TRACE_SNOOP, __VA_ARGS__); \
	fprintf(TRACE_SNOOP, "\n"); }
#else
#define SNOOP_TRACE(...)
#endif

/** @ingroup snoop
 * @def SNOOP_DUMP_NONE
 * SNOOPダンプ: なし */
#define SNOOP_DUMP_NONE     0
/** @ingroup snoop
 * @def SNOOP_DUMP_HEX
 * SNOOPダンプ: Hex表記 */
#define SNOOP_DUMP_HEX      1
/** @ingroup snoop
 * @def SNOOP_DUMP_CHAR
 * SNOOPダンプ: 文字表記 */
#define SNOOP_DUMP_CHAR     2

/* Verbose constants */
/** @ingroup snoop
 * @def SNOOP_VERBOSE_NONE
 * SNOOP VERBOSE: なし */
#define SNOOP_VERBOSE_NONE  0
/** @ingroup snoop
 * @def SNOOP_VERBOSE_ONE
 * SNOOP VERBOSE: レベル1 */
#define SNOOP_VERBOSE_ONE   1
/** @ingroup snoop
 * @def SNOOP_VERBOSE_TWO
 * SNOOP VERBOSE: レベル2 */
#define SNOOP_VERBOSE_TWO   2

/* Filter constants */
/** @ingroup snoop
 * @def SNOOP_FILTER_ALL
 * SNOOPフィルター: すべて */
#define SNOOP_FILTER_ALL	0
/** @ingroup snoop
 * @def SNOOP_FILTER_ARP
 * SNOOPフィルター:  ARP */
#define SNOOP_FILTER_ARP	1
/** @ingroup snoop
 * @def SNOOP_FILTER_IPv4
 * SNOOPフィルター: IPv4 */
#define SNOOP_FILTER_IPv4   2
/** @ingroup snoop
 * @def SNOOP_FILTER_UDP
 * SNOOPフィルター: UDP */
#define SNOOP_FILTER_UDP    3
/** @ingroup snoop
 * @def SNOOP_FILTER_TCP
 * SNOOPフィルター: TCP */
#define SNOOP_FILTER_TCP    4
/** @ingroup snoop
 * @def SNOOP_FILTER_ICMP
 * SNOOP フィルター: ICMP */
#define SNOOP_FILTER_ICMP   5
/** @ingroup snoop
 * @def SNOOP_QLEN
 * SNOOPキューのサイズ */
#define SNOOP_QLEN          100

/** @ingroup snoop
 * @struct snoop
 * snoop構造体 */
struct snoop
{
    uint caplen;                /**< キャプチャするパケットバイト長  */

    bool promisc;               /**< promiscousモードが有効か     */
    uchar type;                 /**< キャプチャするパケットのタイプ  */
    struct netaddr srcaddr;     /**< パケットの送信元アドレス     */
    ushort srcport;             /**< パケットの送信元ポート       */
    struct netaddr dstaddr;     /**< パケットのあて先アドレス     */
    ushort dstport;             /**< パケットのあて先ポート       */

    mailbox queue;              /**< パケットをキューイングするメールボックス */

    uint ncap;                  /**< キャプチャ回数 */
    uint nmatch;                /**< フィルターマッチ回数 */
    uint novrn;                 /**< キャプチャキューのオーバーラン回数 */
    uint nprint;                /**< プリント回数 */
};

/* Function prototypes */
int snoopCapture(struct snoop *cap, struct packet *pkt);
int snoopClose(struct snoop *cap);
bool snoopFilter(struct snoop *cap, struct packet *pkt);
int snoopOpen(struct snoop *cap, char *devname);
int snoopPrint(struct packet *pkt, char dump, char verbose);
int snoopPrintArp(struct arpPkt *arp, char verbose);
int snoopPrintEthernet(struct etherPkt *ether, char verbose);
int snoopPrintIpv4(struct ipv4Pkt *ip, char verbose);
int snoopPrintTcp(struct tcpPkt *tcp, char verbose);
int snoopPrintUdp(struct udpPkt *udp, char verbose);
int snoopPrintDhcp(struct dhcpPkt *dhcp, char verbose);
struct packet *snoopRead(struct snoop *cap);

#endif                          /* _SNOOP_H_ */
