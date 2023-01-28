/**
 * @file memRegionRemove.c
 * Remove a memory region from a region list.
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <safemem.h>

/**
 * リストから既存の領域を削除する
 * @param region 削除する領域
 * @param list 領域が存在するリスト
 */
void memRegionRemove(struct memregion *region, struct memregion **list)
{
    /* region->prevがある場合は、prev->nextをregion->nextに付け替える */
    if ((int)(region->prev) != SYSERR)
    {
        region->prev->next = region->next;
    }
    else
    {
        /* ない場合（先頭）はlistのポインタを更新する */
        *list = region->next;
    }

    /* region->nextがある場合は、netx->prevをregion->prevに付け替える */
    if ((int)(region->next) != SYSERR)
    {
        region->next->prev = region->prev;
    }

    /* この領域の prev/next フィールドをクリアする */
    region->prev = (struct memregion *)SYSERR;
    region->next = (struct memregion *)SYSERR;
}
