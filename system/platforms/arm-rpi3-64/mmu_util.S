/**
 * @file mmu_util.S
 *
 * ARM COretex A53のキャッシュメンテナンス用のアセンブリ関数を含んでいる.
 * @authors Rade Latinovich
 *          Patrick J. McGee
 */
 /* Embedded Xinu, Copyright 2019 */

// xv6のsynchronize.[ch]を参照する
// p15, opcode1, Rd (R/Wレジスタ), CRn (コプロセッサレジスタ), CRm (小プレッサレジスタ), opcode2

.globl _getcacheinfo
_getcacheinfo:
    mrs x0, ccsidr_el1
    ret
    //mrc p15, 1, r0, c0, c0, 0       ;@ Read CCSIDR into r0
    //bx  lr

/* Brief explanation about DMA and Cortex-A53:
 * https://stackoverflow.com/questions/42081435/flush-invalidate-range-by-virtual-address-armv8-cache/
 */

/* DCIMVAC (invalidate specific value)
 * Input: 32-bit virtual address : screenInit.cで使用 */
.globl _inval_area
_inval_area:
    dc ivac, x0
    ret

/* void start_mmu(void); */
.globl start_mmu
start_mmu:
    mrs     x0, sctlr_el1
    orr     x0, x0, #1
    msr     sctlr_el1, x0
    isb
    ret

/* kexec.c で使用 */
.globl stop_mmu
stop_mmu:
    mrs     x0, sctlr_el1
    mov     x1, #1
    bic     x0, x0, x1
    msr     sctlr_el1, x0
    isb
    ret

/* kexec.c で使用 */
.globl invalidate_tlbs
invalidate_tlbs:
    dsb sy
    tlbi vmalle1is
    dsb sy
    ret

.globl PUT32
PUT32:
    str  w1, [x0]
    ret

.globl GET32
GET32:
    ldr  w0, [x0]
    ret

.globl PUT64
PUT64:
    str  x1, [x0]
    ret

.globl GET64
GET64:
    ldr  x0, [x0]
    ret
