/**
 * @file memory.h
 * Definitions for kernel memory allocator and maintenance.
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <stddef.h>

/* roundmb - アドレスをメモリブロックサイズに丸めあげる */
#define roundmb(x)  (void *)( (7 + (ulong)(x)) & ~0x07 )
/* truncmb - アドレスをメモリブロックサイズに切り捨てる */
#define truncmb(x)  (void *)( ((ulong)(x)) & ~0x07 )

/**
 * @ingroup memory_mgmt
 *
 * stkget()で割り当てられたメモリを開放する
 *
 * @param p
 *      割り当てられたスタックの最上位（最大アドレス）のワードへの
 *      ポインタ（stkget()で返されたポインタ）
 * @param len
 *      割り当てられたスタックのサイズ（バイト単位、stkget()に
 *      渡された値と同じ）
 */
#define stkfree(p, len) memfree((void *)((ulong)(p)         \
                                - (ulong)roundmb(len)       \
                                + (ulong)sizeof(ulong)),    \
                                (ulong)roundmb(len))


/**
 * メモリブロックの構造体
 */
struct memblock
{
    struct memblock *next;          /**< 次のメモリブロックへのポインタ */
    uint length;                    /**< メモリブロック（と構造体）のサイズ */
};

extern struct memblock memlist;     /**< フリーメモリリストの先頭          */

/* その他のメモリデータ */

extern void *_end;              /**< リンカが提供するイメージの終端アドレス   */
extern void *_etext;            /**< リンカが提供するテキストセグメント終端アドレス */
extern void *memheap;           /**< ヒープの底                               */

/* メモリ関数プロトタイプ */
void *memget(uint);
syscall memfree(void *, uint);
void *stkget(uint);

#endif                          /* _MEMORY_H_ */
