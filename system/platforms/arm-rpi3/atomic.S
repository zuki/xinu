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
    .func _atomic_increment
    pldw    [r0]
    ldrex   r1, [r0]        // load
    add     r1, #1          // increment
    strex   r2, r1, [r0]    // store
    cmp     r2, #0          // if store fails, try again
    bne     _atomic_increment

    mov     r0, r1          // return incremented value
    dmb

    bx      lr
    .endfunc


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
    .func _atomic_decrement
    pldw    [r0]
    ldrex   r1, [r0]        // load
    sub     r1, #1          // decrement
    strex   r2, r1, [r0]    // store
    cmp     r2, #0          // if store fails, try again
    bne     _atomic_decrement

    mov r0, r1              // return decremented value
    dmb

    bx  lr
    .endfunc


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
    .func _atomic_increment_mod
    pldw    [r0]
    ldrex   r2, [r0]        // load
    add     r2, #1          // increment
    cmp     r2, r1          // if >= to limit, wrap to zero
    movge   r2, #0
    strex   r3, r2, [r0]    // store
    cmp     r3, #0          // if it fails, retry
    bne     _atomic_increment_mod

    mov     r0, r2
    dmb

    bx  lr
    .endfunc
