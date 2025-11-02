/**
 * @file atomic.S
 *
 * アトミックなアセンブリ関数.
 *
 * Embedded Xinu, Copyright (C) 2018. All rights reserved.
 *
 */

.globl _atomic_increment
/**
 * @fn int _atomic_increment(int *var)
 *
 * 指定された整数をアトミックに増分する.
 * (Cの++varに相当する).
 *
 * @param var 変数へのポインタ
 * @return 増分された後に変数に格納されている値.
 */
_atomic_increment:
    .type _atomic_increment, "function"
    pldw    [x0]
1:
    ldaxr   x1, [x0]        // load
    add     x1, x1, #1      // increment
    stlxr   x2, x1, [x0]    // store
    cbnz    x2, 1b          // if store fails, try again

    mov     x0, x1          // return incremented value
    dmb

    ret

.globl _atomic_decrement
/**
 * @fn int _atomic_decrement(int *var)
 *
 * * 指定された整数をアトミックに減分する.
 * (Cの--varに相当する).
 *
 * @param var 変数へのポインタ
 * @return 減分された後に変数に格納されている値.
 */
_atomic_decrement:
    .type _atomic_decrement, "function"
    pldw    [x0]
1:
    ldaxr   x1, [x0]        // load
    sub     x1, x1, #1      // decrement
    stlxr   x2, x1, [x0]    // store
    cbnz    x2, 1b          // if store fails, try again

    mov x0, x1              // return decremented value
    dmb

    ret

.globl _atomic_increment_mod
/**
 * @fn int _atomic_increment_mod(int *var, int mod)
 *
 * 指定された整数をアトミックに増分した後、modする.
 * var = (var + 1) % modに相当する).
 *
 * @param var 変数へのポインタ
 * @param mod varがこの値を超えた場合、ラップアラウンドする値.
 * @return 増分されたvarの値.
 */
_atomic_increment_mod:
    .type _atomic_increment_mod, "function"
    pldw    [x0]
1:
    ldaxr   x2, [x0]        // load
    add     x2, x2, #1      // increment
    cmp     x2, x1          // if >= to limit, wrap to zero
    blt     2f
    mov     x2, #0
2:
    stlxr   x3, x2, [x0]    // store
    cbnz    x3, 1b          // if it fails, retry

    mov     x0, x2
    dmb

    ret
