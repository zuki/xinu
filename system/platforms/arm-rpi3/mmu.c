/**
 * @file mmu.c
 *
 * MMUを定義・有効化する関数の定義.
 *
 * @authors Rade Latinovich
 *          Patrick J. McGee
 */
/* Embedded Xinu, Copyright (C) 2009. All rights reserved */

#include <mmu.h>
#include <mutex.h>
#include <dma_buf.h>

/**
 * @ingroup bcm2837
 *
 * メモリのセクションをキャシュ可能および指定のフラグでマーク付ける.
 * @param vadd 仮想アドレス
 * @param padd 物理アドレス
 * @param flags セクションにマークつけるフラグ
 * @return 0
 */
/* code from Github user dwelch67
 * https://github.com/dwelch67/raspberrypi/tree/master/mmu */
unsigned int mmu_section(unsigned int vadd, unsigned int padd, unsigned int flags)
{
    unsigned int ra, rb, rc;

    ra = vadd >> 20;
    rb = MMUTABLEBASE | (ra << 2);
    rc = (padd & 0xFFF00000) | 0xC00 | flags | 2;
    PUT32(rb, rc);

    return 0;
}

/**
 * @ingroup bcm2837
 *
 * MMUを初期化する.
 * 仮想アドレス == 物理アドレス、および、
 * ペリフェラル領域以外はキャッシュ可能と構成する.
 */
void mmu_init()
{
    unsigned int ra;
    for (ra = 0; ; ra += 0x00100000)
    {
        mmu_section(ra, ra, 0x15C06);
        //mmu_section(ra, ra, 0x0 | 0x8);
        if (ra >= 0x3F000000)
            break;
    }

    /* ペリフェラルはマークを付けない（0x0000を使用） */
    for ( ; ; ra += 0x00100000)
    {
        mmu_section(ra, ra, 0x0000);
        if (ra == 0x40000000)
            break;
    }

    // dmaバッファ領域はキャッシュ不能とする
    mmu_section((uint) dma_buf_space, (uint) dma_buf_space, 0x0);

    start_mmu(MMUTABLEBASE);
}
