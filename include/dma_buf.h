/**
 * @file dma_buf.h
 * DMAバッファの割り当てと維持のための定義.
 *
 */
/* Embedded Xinu, Copyright (C) 2019. All rights reserved. */

#ifndef _DMA_BUF_H_
#define _DMA_BUF_H_

#include <stddef.h>
#include <stdint.h>

extern uint8_t dma_buf_space[];

/* DMAバッファ関数プロトタイプ */
syscall dma_buf_init(void);
void *dma_buf_alloc(uint);
syscall dma_buf_free(void *, uint);

#endif	/* _DMA_BUF_H_ */
