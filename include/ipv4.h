/**
 * @file ipv4.h
 * @ingroup ipv4
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _IPv4_H_
#define _IPv4_H_

#include <stddef.h>
#include <network.h>
#include <stdint.h>

/** トレース用マクロ */
//#define TRACE_IPv4     TTY1
#ifdef TRACE_IPv4
#include <stdio.h>
#define IPv4_TRACE(...)     { \
		fprintf(TRACE_IPv4, "%s:%d (%d) ", __FILE__, __LINE__, gettid()); \
		fprintf(TRACE_IPv4, __VA_ARGS__); \
		fprintf(TRACE_IPv4, "\n"); }
#else
#define IPv4_TRACE(...)
#endif

/** ドット10進記法のIPv4アドレスの最大長 */
#define IPv4_DOTDEC_MAXLEN    15

/** パケットのセクションを指すマスク */
#define IPv4_IHL			0x0F
#define IPv4_VER			0xF0
#define IPv4_FLAGS		    0xE000
#define IPv4_FROFF		    0x1FFF
#define IPv4_ADDR_LEN       4
#define IPv4_TTL 			64

/** IPパケット定義 */
#define IPv4_HDR_LEN     	20      /* オプションとパディングはないものと仮定 */
#define IPv4_MAX_OPTLEN		40
#define IPv4_MAX_HDRLEN 	IPv4_HDR_LEN + IPv4_MAX_OPTLEN
#define IPv4_MIN_IHL		5
#define IPv4_MAX_IHL		15
#define IPv4_VERSION		4

/* IPプロトコル */
#define IPv4_PROTO_ICMP     1
#define IPv4_PROTO_IGMP     2
#define IPv4_PROTO_TCP	    6
#define IPv4_PROTO_UDP      17

/* フラグ */
#define IPv4_FLAG_LF		0x0000
#define IPv4_FLAG_MF 		0x2000
#define IPv4_FLAG_DF 		0x4000

/* サービスのタイプ */
#define IPv4_TOS_NETCNTRL	0x7
#define IPv4_TOS_INTCNTRL	0x6
#define IPv4_TOS_CRITIC	    0x5
#define IPv4_TOS_FLASHOV	0x4
#define IPv4_TOS_FLASH	    0x3
#define IPv4_TOS_IM		    0x2
#define IPv4_TOS_PRIO		0x1
#define IPv4_TOS_ROUTINE	0x0

/**
 * @ingroup ipv4
 *
 * @brief IPv4ヘッダー.
 *
 * \code
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Link-Level Header                                             |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Version| IHL   |Type of Service| Total Length (IHL + Data)     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |         Identification        |Flags|    Fragment Offset      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Time to Live  |     Protocol  |       Header Checksum         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         Source Address                        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                       Destination Address                     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |              Options  & Padding (Variable octets)             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Data (Variable octets)                                        |
 * | ...                                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \endcode
 */
struct ipv4Pkt
{
    uint8_t   ver_ihl;            /**<  */
    uint8_t   tos;                /**< IPv4サービスタイプ                    */
    uint16_t  len;                /**< IPv4パケット長（IHLを含む）           */
    uint16_t  id;                 /**< IPv4識別子                            */
    uint16_t  flags_froff;        /**< IPv4フラグとフラグメントオフセット    */
    uint8_t   ttl;                /**< IPv4 TTL (time to live)               */
    uint8_t   proto;              /**< IPv4プロトコル                        */
    uint16_t  chksum;             /**< IPv4チェックサム                      */
    uint8_t   src[IPv4_ADDR_LEN]; /**< IPv4送信元                            */
    uint8_t   dst[IPv4_ADDR_LEN]; /**< IPv4宛先                              */
    uint8_t   opts[1];            /**< オプションとパディングは可変          */
};

/** 関数プロトタイプ */
syscall dot2ipv4(const char *, struct netaddr *);
syscall ipv4Recv(struct packet *);
bool ipv4RecvValid(struct ipv4Pkt *);
bool ipv4RecvDemux(struct netaddr *);
syscall ipv4Send(struct packet *, struct netaddr *, struct netaddr *,
                 uchar);
syscall ipv4SendFrag(struct packet *, struct netaddr *);

#endif                          /* _IPv4_H_ */
