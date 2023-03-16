/**
 * @file snoopDns.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <stdint.h>
#include <network.h>
#include <stdio.h>
#include <string.h>
#include <snoop.h>
#include <dns.h>

static void snoopPrintDnsQr(uint8_t qr, char *descrp)
{
    switch (qr)
    {
    case DNS_Q:
        strcpy(descrp, "(Query)");
        break;
    case DNS_R:
        strcpy(descrp, "(Response)");
        break;
    default:
        strcpy(descrp, "");
        break;
    }
}

static void snoopPrintDnsOpcode(uint8_t opcode, char *descrp)
{
    switch (opcode)
    {
    case DNS_CODE_Q:
        strcpy(descrp, "(Query)");
        break;
    case DNS_CODE_IQ:
        strcpy(descrp, "(Inverse Query)");
        break;
    case DNS_CODE_STATUS:
        strcpy(descrp, "(Status)");
        break;
    default:
        strcpy(descrp, "");
        break;
    }
}

static void snoopPrintDnsRcode(uint8_t rcode, char *descrp)
{
    switch (rcode)
    {
    case DNS_RCODE_NOERROR:
        strcpy(descrp, "(No Error)");
        break;
    case DNS_RCODE_FORMERR:
        strcpy(descrp, "(Format Error)");
        break;
    case DNS_RCODE_SERVFAIL:
        strcpy(descrp, "(Server Error)");
        break;
    case DNS_RCODE_NXDOMAIN:
        strcpy(descrp, "(Name Error)");
        break;
    case DNS_RCODE_NOTIMP:
        strcpy(descrp, "(Not Implemented Error)");
        break;
    case DNS_RCODE_REFUSED:
        strcpy(descrp, "(Refused)");
        break;
    default:
        strcpy(descrp, "");
        break;
    }
}

static void snoopPrintDnsQtype(uint16_t qtype, char *descrp)
{
    switch (qtype)
    {
    case DNS_QT_A:
        strcpy(descrp, "(Host Address)");
        break;
    case DNS_QT_NS:
        strcpy(descrp, "(Name Server)");
        break;
    case DNS_QT_CNAME:
        strcpy(descrp, "(Canonical Name)");
        break;
    case DNS_QT_SOA:
        strcpy(descrp, "(Start of Zone)");
        break;
    case DNS_QT_PTR:
        strcpy(descrp, "(Domain Name Pointer)");
        break;
    case DNS_QT_HINFO:
        strcpy(descrp, "(Host Information)");
        break;
    case DNS_QT_MX:
        strcpy(descrp, "(Mail Exchange)");
        break;
    case DNS_QT_TXT:
        strcpy(descrp, "(Text Strings)");
        break;
    case DNS_QT_AXFR:
        strcpy(descrp, "(Request Transfer Zone)");
        break;
    case DNS_QT_ALL:
        strcpy(descrp, "(Reqeust for All Records)");
        break;
    default:
        strcpy(descrp, "");
        break;
    }
}

static void snoopPrintDnsQclass(uint16_t qclass, char *descrp)
{
    switch (qclass)
    {
    case DNS_QC_IN:
        strcpy(descrp, "(Internet)");
        break;
    case DNS_QC_ANY:
        strcpy(descrp, "(Any Class)");
        break;
    default:
        strcpy(descrp, "");
        break;
    }
}

static void snoopPrintRR(struct dnsPkt *dns, char **ptr, uint16_t count, char *title)
{
    uint32_t dnamelen;
    struct dns_rr *rr;
    char descrp[40];
    char output[40];
    char dname[DNS_DATASIZE];

    for (int i = 0; i < count; i++)
    {
        printf(" ----- %s %d -----\n", title, i);
        dnamelen = dnsGetRName((char *)dns, (char *)*ptr, dname);
        printf("  Domain Name: %s\n", dname);
        rr = (struct dns_rr *)(*ptr + dnamelen);
        snoopPrintDnsQtype(net2hs(rr->rtype), descrp);
        sprintf(output, "%u %s", net2hs(rr->rtype), descrp);
        printf("  Record Type: %-35s ", output);
        snoopPrintDnsQclass(net2hs(rr->rclass), descrp);
        sprintf(output, "%u %s", net2hs(rr->rclass), descrp);
        printf("  Class: %-25s\n", output);
        sprintf(output, "0x%08x (%u)", rr->ttl, net2hs(rr->ttl));
        printf("  TTL: %-25s ", output);
        sprintf(output, "0x%04x (%u)", rr->rdlen, net2hs(rr->rdlen));
        printf("  RD length: %-25s\n", output);
        *ptr += dnamelen + 4 + 6;
        switch(net2hs(rr->rtype)) {
            case DNS_QT_A:
                //addr = *(uint32_t *)(*ptr);
                sprintf(output, "%d.%d.%d.%d", *(*ptr), *(*ptr+1), *(*ptr+2), *(*ptr+3));
                //    (uint8_t)((addr >> 24) & 0xff), (uint8_t)((addr >> 16) & 0xff),
                //    (uint8_t)((addr >> 8) & 0xff), (uint8_t)((addr >> 0) & 0xff));
                printf("  RData: %-25s\n", output);
                *ptr += 4;
                break;
            case DNS_QT_CNAME:
            case DNS_QT_PTR:
                dnamelen = dnsGetRName((char *)dns, (char *)*ptr, dname);
                printf("  RDATA: %-25s\n", dname);
                *ptr += dnamelen;
                break;
            default:
                printf("  RDATA: Unrecognized\n");
                *ptr += net2hs(rr->rdlen);
                break;
        }
    }
}


/**
 * @ingroup snoop
 * DNSパケットの内容を出力する.
 * @param dns DNSパケット
 * @param verbose 詳細レベル
 * @return 出力に成功した場合は ::OK; エラーが発生した場合は ::SYSERR
 */
int snoopPrintDns(struct dnsPkt *dns, char verbose)
{
    char descrp[40];
    char output[40];
    char dname[DNS_DATASIZE];
    uint32_t dnamelen = 0;
    struct dns_q *q;
    char *ptr = NULL;

    if (NULL == dns) {
        return SYSERR;
    }

    printf(" ----- DNS Header -----\n");
    sprintf(output, "0x%04x (%u)", dns->id, net2hs(dns->id));
    printf("  ID: %-15s ", output);
    snoopPrintDnsQr(dns->qr, descrp);
    sprintf(output, "%u %s", dns->qr, descrp);
    printf("  QR: %-15s ", output);
    snoopPrintDnsOpcode(dns->opcode, descrp);
    sprintf(output, "%u %s", dns->opcode, descrp);
    printf("  Opcode: %-25s\n", output);
    sprintf(output, "%u %s", dns->aa, dns->aa == 1 ? "(YES)" : "NO");
    printf("  AA: %-15s ", output);
    sprintf(output, "%u %s", dns->tc, dns->tc == 1 ? "(YES)" : "NO");
    printf("  TC: %-15s ", output);
    sprintf(output, "%u %s", dns->rd, dns->rd == 1 ? "(YES)" : "NO");
    printf("  RD: %-15s\n", output);
    sprintf(output, "%u %s", dns->ra, dns->ra == 1 ? "(YES)" : "NO");
    printf("  RA: %-15s ", output);
    snoopPrintDnsRcode(dns->rcode, descrp);
    sprintf(output, "%u %s", dns->rcode, descrp);
    printf("  RCODE: %-25s\n", output);

    printf("  QUC: %d ", net2hs(dns->qucount));
    printf("  ANC: %d ", net2hs(dns->ancount));
    printf("  NSC: %d ", net2hs(dns->nscount));
    printf("  ARC: %d\n", net2hs(dns->arcount));

    ptr = dns->data;

    for (int i = 0; i < net2hs(dns->qucount); i++)
    {
        printf(" ----- Question Section %d -----\n", i);
        dnamelen = dnsGetRName((char *)dns, (char *)ptr, dname);
        printf("  Question Name: %s\n", dname);
        q = (struct dns_q *)(ptr + dnamelen);
        snoopPrintDnsQtype(net2hs(q->qtype), descrp);
        sprintf(output, "%u %s", net2hs(q->qtype), descrp);
        printf("  Type: %-25s ", output);
        snoopPrintDnsQclass(net2hs(q->qclass), descrp);
        sprintf(output, "%u %s", net2hs(q->qclass), descrp);
        printf("  Class: %-25s\n", output);
        ptr += dnamelen + 4;
    }

    snoopPrintRR(dns, &ptr, net2hs(dns->ancount), "Answer Section");
    snoopPrintRR(dns, &ptr, net2hs(dns->nscount), "Authority Section");
    snoopPrintRR(dns, &ptr, net2hs(dns->arcount), "Additional Section");

    return OK;
}
