/**
 * @file kexec.c
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <interrupt.h>
#include <kernel.h>
#include <kexec.h>
#include <string.h>


/* 以下の配列は、新しいカーネルを最終的な場所にコピーし、制御を渡すために
 * 使用されるARM命令のスタブである。
 *
 * このスタブは配列としてハードコードされているが、これは新しいカーネルを
 * コピーする際にそれ自身によって上書きされない場所にコピーされる必要がある
 * ためである。したがって、そのサイズは既知である必要があり、完全に再配置
 * 可能である必要がある（理論的にはアセンブラは保証していない）。
 *
 * 引数は以下の通り。
 *
 * r0: 新しいカーネルへのポインタ
 * r1：新規カーネルのサイズ（32ビットワード単位）
 * r2: ARMブートタグへのポインタ (新しいカーネルの便宜上、r2に保存される)
 *
 * これはカーネルをアドレス0x8000にコピーするようにハードコーディングされている。
 */

/*00000000 <copy_kernel>:*/
  /* 0:   e3a04902    mov     r4, #32768        ; 0x8000 */
  /* 4:   e4903004    ldr     r3, [r0], #4               */
  /* 8:   e4843004    str     r3, [r4], #4               */
  /* c:   e2511001    subs    r1, r1, #1                 */
  /*10:   1afffffb    bne     4 <copy_kernel+0x4>        */
  /*14:   e3a0f902    mov     pc, #32768        ; 0x8000 */
static const unsigned long copy_kernel[] = {
    0xe3a04902,
    0xe4903004,
    0xe4843004,
    0xe2511001,
    0x1afffffb,
    0xe3a0f902,
};

#define COPY_KERNEL_ADDR ((void*)(0x8000 - sizeof(copy_kernel)))

/**
 * カーネルの実行 - 制御を新しいカーネルに渡す.
 *
 * これはRaspberry Piの実装である。この実装では、新しいカーネルは
 * Raspberry Pi用として妥当であり、アドレス 0x8000で実行するように
 * リンクされ、アドレス 0x8000をエントリポイントとして持つ。
 *
 * @param kernel
 *      メモリのどこかにロードされている新しいカーネルイメージへのポインタ
 * @param size
 *      新しいカーネルイメージのバイト単位のサイズ
 *
 * @return
 *      この関数は復帰しない。何らかの形で復帰してしまった場合は何かが
 *      ひどく間違っている。
 */
syscall kexec(const void *kernel, uint size)
{
    irqmask im;

    im = disable();

    /* アセンブリスタブを安全な場所にコピーする  */
    memcpy(COPY_KERNEL_ADDR, copy_kernel, sizeof(copy_kernel));

    /* アセンブリスタブを実行して、新しいカーネルを最終的な場所に
     * コピーし、制御を渡す */
    extern void *atags_ptr;
    (( void (*)(const void *, unsigned long, void *))(COPY_KERNEL_ADDR))
                (kernel, (size + 3) / 4, atags_ptr);

    /* 制御は絶対にここには来ないはず */
    restore(im);
    return SYSERR;
}
