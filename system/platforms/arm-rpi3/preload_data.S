/**
 * @file preload_data.S
 * pldw を提供する.
*/
.globl pldw

/**
 * @ingroup bcm2837
 * @fn void pldw(unsigned int *var)
 *
 * pldw 命令のラップ関数.
 *
 * @param var preload writeする変数へのポインタ
 */
pldw:
    pldw [r0]
    bx   lr
