/**
 * @file mmu.c
 *
 * MMUを定義・有効化する関数の定義.
 *
 * @authors Rade Latinovich
 *          Patrick J. McGee
 */
/* Embedded Xinu, Copyright (C) 2009. All rights reserved */

#include <stdint.h>
#include <mmu_64.h>
#include <mutex.h>
#include <dma_buf.h>

extern uint64_t kpte[512];

/**
 * @ingroup bcm2837
 *
 * メモリセクションをキャシュ可能および指定のフラグでマーク付ける.
 * @param vadd 仮想アドレス
 * @param padd 物理アドレス
 * @param flags セクションにマークつけるフラグ
 * @return 0
 */
/* https://www.aps-web.jp/academy/ca/228/ 参照 */
/* code from Github user dwelch67
 * https://github.com/dwelch67/raspberrypi/tree/master/mmu */
unsigned int mmu_section(uint64_t vadd, uint64_t padd, uint32_t flags)
{
    uint32_t idx, flg;
    uint64_t pte;

    if (vadd >= 0x40000000UL)
        return SYSERR;
    
    idx = (uint32_t)((vadd >> 20) & 0x1ff);
    flg = PTE_FLAGS(kpte[idx]);
    if (flg != PTE_KDATA && flg != PTE_KDMA)
        return SYSERR;

    pte = (PTE_ADDR(kpte[idx]) & ~0x3ff) | flags;
    kpte[idx] = pte;

    return 0;
}

/**
 * @ingroup bcm2837
 *
 * MMUを初期化する. 1ページ1MBの恒等マップ。
 * 仮想アドレス == 物理アドレス、および、
 * ペリフェラル領域以外はキャッシュ可能と構成する.
 */
void mmu_init()
{
    // dmaバッファ領域はキャッシュ不能とする
    mmu_section((uint64_t) dma_buf_space, (uint64_t) dma_buf_space, PTE_KDMA);
    // system/platform/arm-rpi3-64/mmu_util.S にあり
    start_mmu();
}
