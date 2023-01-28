/**
 * @file arpGetEntry.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <arp.h>
#include <clock.h>
#include <interrupt.h>

/**
 * @ingroup arp
 *
 * ARPテーブルから指定したプロトコルアドレスのエントリを取得する
 * @param praddr プロトコルアドレス
 * @return epraddrに対応するARPテーブルのエントリ、
 * 存在しない場合は NULL
 */
struct arpEntry *arpGetEntry(const struct netaddr *praddr)
{
    int i = 0;
    struct arpEntry *entry = NULL;  /**< ARPテーブルエントリへのポインタ   */
    irqmask im;                     /**< 割り込み状態              */

    ARP_TRACE("Getting ARP entry");
    im = disable();

    /* ARPテーブルを走査する */
    for (i = 0; i < ARP_NENTRY; i++)
    {
        /* ARPエントリが未使用の場合は次のエントリにスキップする */
        if (!(ARP_USED & arptab[i].state))
        {
            continue;
        }

        entry = &arptab[i];
        /* エントリがタイムアウトでないかチェックする */
        if (entry->expires < clktime)
        {
            ARP_TRACE("\tEntry %d expired", i);
            arpFree(entry);
            continue;
        }

        /* プロトコルタイプとアドレスがマッチするかチェックする */
        if (netaddrequal(&entry->praddr, praddr))
        {
            restore(im);
            ARP_TRACE("\tEntry %d matches", i);
            return entry;
        }
    }

    restore(im);
    ARP_TRACE("\tNo entry matches");
    return NULL;
}
