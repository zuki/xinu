#include <arm.h>
#include <mmu.h>

/**
 * @fn void CoreSetup(void) __attribute__((naked))
 *
 */
.globl CoreSetup
CoreSetup:
    /* プロセッサをSYSTEMモードに変える */
    mrs  r0, cpsr
    orr  r0, r0, #ARM_MODE_SYS
    msr  spsr_cxsf, r0
    add  r0, pc, #4
    msr  ELR_hyp, r0
    eret

    /* ベクタアドレスをセットする */
    ldr  r1, =_vectors
    mcr  p15, 0, r1, c12, c0, 0     /* Write to VBAR */

    /* MPIDRレジスタからCPUIDを取得してr0に置く */
    mrc  p15, 0, r0, c0, c0, 5      /* MPIDR */
    and  r0, r0, #7                 /* last 3 bits is CPUID */

    /* CPUIDに4をかけて、配列の正しいインデックスを取得する */
    mov  r1, r0
    mov  r2, #4
    mul  r1, r1, r2

    /* core_init_sp配列は各コア用の初期スタックポインタ
     * system/p/unparkcore.cで定義して、loader/p/a/start.Sで設定
     */
    ldr  r2, =core_init_sp
    ldr  sp, [r2, r1]

    /* start_mmuを呼ぶ前に非volatileなレジスタにコアindex値をmove */
    mov  r4, r1

    /* コアをアンパークするたびにプログラマが手動で呼び出す必要が
     * ないように、start_mmuを呼び出す */
    mov  r0, #MMUTABLEBASE      /* MMUTABLEBASE from mmu.h */
    bl   start_mmu

    /* corestart: セットアップ完了後に実行する各コア用の命令アドレスの配列 */
    ldr  r0, =init_args
    ldr  r0, [r0, r4]
    ldr  r2, =corestart
    ldr  pc, [r2, r4]


/* EVentルーチンを送信し、Cコードで使用できるようにする (unparkcore.c) */
.globl sev
sev:
    .func sev
    sev
    bx  lr
    .endfunc
