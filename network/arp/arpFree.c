/**
 * @file arpFree.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <arp.h>
#include <interrupt.h>
#include <stdlib.h>

/**
 * @ingroup arp
 *
 * ARPテーブルのエントリを解放する
 * @return エラーが発生した場合は SYSERR、それ以外は OK
 */
syscall arpFree(struct arpEntry *entry)
{
    ARP_TRACE("Freeing ARP entry");

    /* ポインタのエラーチェック */
    if (NULL == entry)
    {
        return SYSERR;
    }

    /* 解決を待っているスレッドに通知する */
    if (ARP_UNRESOLVED == entry->state)
    {
        arpNotify(entry, TIMEOUT);
        ARP_TRACE("Waiting threads notified");
    }

    /* ARPテーブルエントリをクリアする */
    bzero(entry, sizeof(struct arpEntry));
    entry->state = ARP_FREE;
    ARP_TRACE("Freed entry %d",
              ((int)entry - (int)arptab) / sizeof(struct arpEntry));
    return OK;
}
