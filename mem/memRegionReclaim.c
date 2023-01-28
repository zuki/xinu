/**
 * @file     memRegionReclaim.c
 * Attempt to reclaim an allocated memory region.
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <safemem.h>

/**
 * 指定されたスレッドIDについて、そのスレッドに割り当てられていた
 * すべてのメモリ領域を回収してregion freeリストに返す。
 * @param tid メモリ領域を保持しているスレッドID
 */
void memRegionReclaim(tid_typ tid)
{
    struct memregion *region, *nextregion;

    /* tidに割り当てた領域を開放する */
    region = regalloclist;
    while ((int)region != SYSERR)
    {
        /* 変更する前に次の領域を保存 */
        nextregion = region->next;

        if (region->thread_id == tid)
        {
            // 割り当てリストから削除
            memRegionRemove(region, &regalloclist);
            // ページをアンマップ
            safeUnmapRange(region->start, region->length);
            // フリーリストに挿入
            memRegionInsert(region, &regfreelist);
        }

        region = nextregion;
    }
}
