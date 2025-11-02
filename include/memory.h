/**
 * @file memory.h
 * Definitions for kernel memory allocator and maintenance.
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <stddef.h>
#include <stdint.h>

#define MBSIZE  (sizeof(struct memblock) - 1)
/* roundmb - アドレスをメモリブロックサイズに丸めあげる */
#define roundmb(x)  (void *)( (MBSIZE + (uintptr_t)(x)) & ~MBSIZE )
/* truncmb - アドレスをメモリブロックサイズに切り捨てる */
#define truncmb(x)  (void *)( ((uintptr_t)(x)) & ~MBSIZE )
/* roundvalue - 値をメモリブロックサイズに丸めあげる */
#define roundvalue(x)  ( (MBSIZE + (uintptr_t)(x)) & ~MBSIZE )

/**
 * @ingroup memory_mgmt
 *
 * stkget()で割り当てられたメモリを解放する
 *
 * @param p
 *      割り当てられたスタックの最上位（最大アドレス）のワードへの
 *      ポインタ（stkget()で返されたポインタ）
 * @param len
 *      割り当てられたスタックのサイズ（バイト単位、stkget()に
 *      渡された値と同じ）
 */
#define stkfree(p, len) memfree((void *)((uint64_t)(p)         \
                                - (uint64_t)roundvalue(len)       \
                                + (uint64_t)sizeof(uint64_t)),    \
                                (uint64_t)roundvalue(len))


/**
 * メモリブロックの構造体
 */
struct memblock
{
    struct memblock *next;          /**< 次のメモリブロックへのポインタ */
    uint32_t length;                /**< メモリブロック（と構造体）のサイズ */
};

extern struct memblock memlist;     /**< フリーメモリリストの先頭          */

/* その他のメモリデータ */

extern void *_end;              /**< リンカが提供するイメージの終端アドレス   */
extern void *_etext;            /**< リンカが提供するテキストセグメント終端アドレス */
extern void *memheap;           /**< ヒープの底 */

/* メモリ関数プロトタイプ */
void *memget(uint32_t);
syscall memfree(void *, uint32_t);
void *stkget(uint32_t);

#endif                          /* _MEMORY_H_ */
