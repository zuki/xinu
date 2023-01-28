/**
 * @file safeUnmap.c
 * Unmap a page from the page table.
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <safemem.h>
#include <mips.h>
#include <stdlib.h>

/**
 * ページテーブルからページをアンマップする
 * @param page ページテーブルから削除するページ
 * @return 失敗の場合は非ゼロ値
 */
int safeUnmap(void *page)
{
    int index;
    struct pgtblent *entry;

    if (NULL == pgtbl || NULL == page)
    {
        return 1;               /* invalid */
    }

    /* ページテーブル園t理のインデックスを計算 */
    index = (((uint)page & PMEM_MASK) >> 12);
    entry = &(pgtbl[index]);

    /* エントリが正しくメッピングされているか確認する */
    if ((entry->entry & 0xffffffc0) != ((uint)page >> 6))
    {
        return 1;               /* corrupted page */
    }

    /* ページテール部からエントリをクリアする */
    bzero(entry, sizeof(struct pgtblent));

    return 0;
}
