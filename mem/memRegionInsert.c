/**
 * @file memRegionInsert.c
 * Insert a memory region into a list of memory regions (free or
 * allocated).
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <safemem.h>

extern struct memregion *regalloclist, *regfreelist;
static void memRegionCoalesce(struct memregion *region);

/**
 * regionをリストに挿入する。（リストがフリーリストの場合）、
 * 該当すれば、近隣の地域と合体する
 * @param region 挿入する領域
 * @param list 領域を挿入するリスト
 */
void memRegionInsert(struct memregion *region, struct memregion **list)
{
    struct memregion *current, *previous;

    /* リストに領域があるか調べる。なければ単に追加する */
    if (SYSERR == (int)(*list))
    {
        *list = region;

        return;
    }

    /* regionのアドレスより大きなアドレスを持つ既存の領域を探す */
    current = *list;
    while ((SYSERR != (int)current) && (current->start < region->start))
    {
        /* Iterate */
        previous = current;
        current = current->next;
    }

    if (SYSERR == (int)current)
    {
        /* currentの後に追加する: どの領域のアドレスより大きい場合 */
        previous->next = region;
        region->prev = previous;
        region->next = (struct memregion *)SYSERR;
    }
    else
    {
        /* currentの前に追加する: curr.prev < region < curr */
        region->prev = current->prev;
        region->next = current;
        if ((int)(current->prev) != SYSERR) // currentは先頭ではない
        {
            current->prev->next = region;
        }
        else
        {
            *list = region;     // currentが先頭だったのでregionを先頭に
        }
        current->prev = region;
    }

    /* フリーリストの領域であれば合体する */
    if (*list == regfreelist)
    {
        memRegionCoalesce(region);
    }
}

/**
 * 隣接するメモリ領域を一つの大きな領域に合体する。
 * これにより連続する大きなスペースが存在するようになれば
 * 大きな領域を割り当てられるようになる
 * @param region 合体を試みる領域のポインタ
 */
static void memRegionCoalesce(struct memregion *region)
{
    struct memregion *adjacent;

    /* regionはregion->nextと隣接するか? */
    adjacent = region->next;
    if (((int)adjacent != SYSERR)
        && ((uint)(region->start) + region->length)
        == (uint)(adjacent->start))
    {
        /* regionとregion->nextを合体 */
        region->length += adjacent->length;
        region->next = adjacent->next;
        if (SYSERR != (int)adjacent->next)
        {
            adjacent->next->prev = region;
        }
    }

    /* region->prevはregionと隣接するか? */
    adjacent = region->prev;
    if (((int)adjacent != SYSERR)
        && ((uint)(adjacent->start) + adjacent->length ==
            (uint)(region->start)))
    {
        /* regionとregion->prevを合体 */
        adjacent->length += region->length;
        adjacent->next = region->next;
        if (SYSERR != (int)region->next)
        {
            region->next->prev = adjacent;
        }
    }
}
