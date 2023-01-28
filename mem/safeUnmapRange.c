/**
 * @file  safeUnmapRange.c
 * Remove a range of pages from page table.
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <safemem.h>

/**
 * ページテーブルからstartから始まり、lengthバイト後に終わる
 * ページ領域を削除する
 * @param start 削除する先頭のページアドレス
 * @param length アンマップするバイト数
 * @return 失敗した場合は 非ゼロ値
 */
int safeUnmapRange(void *start, uint length)
{
    uint addr, end;
    int result;

    length = roundpage(length);
    addr = (uint)truncpage(start);
    end = addr + length;

    for (; addr < end; addr += PAGE_SIZE)
    {
        if ((result = safeUnmap((void *)addr)) != 0)
        {
            return result;
        }
    }

    return 0;
}
