/**
 * @file bufpool.h
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#ifndef _BUFPOOL_H_
#define _BUFPOOL_H_

#include <stddef.h>
#include <semaphore.h>
#include <conf.h>

/* バッファプールの状態定義 */
#define BFPFREE  1
#define BFPUSED  2

/**
 * 個々のプールバッファ
 */
struct poolbuf
{
    struct poolbuf *next;
    int poolid;
};

/**
 * バッファプールテーブルエントリ
 */
struct bfpentry
{
    uchar state;
    uint bufsize;
    uint nbuf;
    void *head;
    struct poolbuf *next;
    semaphore freebuf;
};

/**
 * isbadpool - 指定されたバファプールidと状態の妥当性を・テストする
 * @param p テストするid番号
 */
#define isbadpool(p) ((p >= NPOOL)||(p < 0)||(BFPFREE == bfptab[p].state))

/** roundword - 指定されたバイト数をワードサイズに丸める
 *  @param b バイト単位のサイズ
 */
#define roundword(b) ((3 + b) & ~0x03)

extern struct bfpentry bfptab[];

/* 関数プロトタイプ */
void *bufget(int);
syscall buffree(void *);
int bfpalloc(uint, uint);
syscall bfpfree(int);

#ifndef NPOOL
#  define NPOOL 0
#endif

/* プールが定義されていない場合のダミー定義  */
#if NPOOL == 0
#  ifndef POOL_MAX_BUFSIZE
#    define POOL_MAX_BUFSIZE 0
#  endif
#  ifndef POOL_MIN_BUFSIZE
#    define POOL_MIN_BUFSIZE 0
#  endif
#  ifndef POOL_MAX_NBUFS
#    define POOL_MAX_NBUFS 0
#  endif
#endif

#endif                          /* _BUFPOOL_H_ */
