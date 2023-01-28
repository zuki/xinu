/**
 * @file arpAlloc.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <arp.h>
#include <stdlib.h>

/**
 * @ingroup arp
 *
 * ARPテーブルからエントリを割り当てる。
 * @return ARPテーブルのエントリ、エラーが発生した場合は SYSERR
 * @pre-condition 割り込みは無効であること
 * @post-condition 割り込みは依然として無効であること
 */
struct arpEntry *arpAlloc(void)
{
    struct arpEntry *minexpires = NULL;
    int i = 0;

    ARP_TRACE("Allocating ARP entry");

    for (i = 0; i < ARP_NENTRY; i++)
    {
        /* エントリが未使用の場合、そのエントリを返す */
        if (ARP_FREE == arptab[i].state)
        {
            arptab[i].state = ARP_USED;
            ARP_TRACE("\tFree entry %d", i);
            return &arptab[i];
        }

        if ((NULL == minexpires)
            || (arptab[i].expires < minexpires->expires))
        {
            minexpires = &arptab[i];
            ARP_TRACE("\tMinexpires entry %d, expires %u", i,
                      minexpires->expires);
        }
    }

    /* 未使用または最小の有効期限切れのエントリがない場合はエラー */
    if (NULL == minexpires)
    {
        ARP_TRACE("\tNo free or minexpires entry");
        return (struct arpEntry *)SYSERR;
    }

    /* 最小の有効期限切れのエントリを返す */
    bzero(minexpires, sizeof(struct arpEntry));
    minexpires->state = ARP_USED;
    return minexpires;
}
