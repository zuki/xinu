#include <sysregs_64.h>
#include <mmu_64.h>

/**
 * @fn void CoreSetup(void) __attribute__((naked))
 *
 */
.globl CoreSetup
CoreSetup:
    /* 現在の例外レベルを取得して、対応する処理に分岐する */
    mrs     x9, CurrentEL
    and     x9, x9, #0xc
    cmp     x9, #8
    beq     el2
    bgt     el3

el3:
    mov     x9, #SCR_VALUE
    msr     scr_el3, x9
    mov     x9, #SPSR_EL3_VALUE
    msr     spsr_el3, x9
    adr     x9, el2
    msr     elr_el3, x9
    eret

el2:
    /*
     * HCR_EL2.RW, bit[31] = 1: The Execution state for EL1 is AArch64.
     * The Execution state for EL0 is determined by the current
     * value of PSTATE.nRW when executing at EL0.
     */
    mov     x9, #HCR_VALUE
    msr     hcr_el2, x9

    /* Setup SCTLR access. */
    ldr     x9, =SCTLR_VALUE_MMU_DISABLED
    msr     sctlr_el1, x9

    /* Enable SIMD instructions. */
    ldr     x9, =CPACR_VALUE
    msr     cpacr_el1, x9

    /* Change execution level to EL1. */
    mov     x9, #SPSR_EL2_VALUE
    msr     spsr_el2, x9
    adr     x9, el1
    msr     elr_el2, x9
    eret

el1:
    /* disable fiq, irq */
    msr daifset, #3

    adr     x9, kpgdir
 
    /* Higher and lower half map to same physical memory region. */
    msr     ttbr0_el1, x9
    msr     ttbr1_el1, x9
 
    ldr     x9, =TCR_VALUE
    msr     tcr_el1, x9
 
    ldr     x9, =MAIR_VALUE
    msr     mair_el1, x9
 
    isb
 
    /* MMUを有効にする. I/DキャッシュはON設定になっているので */
    /* SCTLR_MMU_ENABLEDフラグを立てるだけ */
    mrs     x9, sctlr_el1
    orr     x9, x9, #SCTLR_MMU_ENABLED
    msr     sctlr_el1, x9

    isb
    
    /* VBARに例外テーブルのアドレスを書き込む */
    ldr x0, =vectors
    msr vbar_el1, x0

    /* 各コアのspをセットする。ore_init_sp配列は各コア用の初期スタックポインタ
     * system/p/unparkcore.cで定義して、loader/p/a/start.Sで設定 */
    mrs  x0, mpidr_el1
    and  x0, x0, #3
    ldr  x1, =core_init_sp
    ldr  x2, [x1, x0, lsl #3]
    mov  sp, x2

    /* コアをアンパークするたびにプログラマが手動で呼び出す必要が
     * ないように、start_mmuを呼び出す */
    //bl   start_mmu

    /* corestart. init_args: 各コアのセットアップ完了後に実行する
     *  関数とその引数: unparkcore()でセットされちる
     *  pcに関数アドレスをロードすることで実行が開始される */
    ldr  x0, =init_args
    ldr  x0, [x0, x4]
    ldr  x2, =corestart
    br   x2
