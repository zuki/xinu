/**
 * @file mutex.S
 * _mutex_acquire, _mutex_release, _atomic_mutex_check を提供する.
 *
 * Written by:  Rade Latinovich
 *              Patrick McGee
 */

#include <mutex.h>

.globl _mutex_acquire
.globl _mutex_release
.globl _atomic_mutex_check

/**
 * @fn void _mutex_acquire(unsigned int *mutex)
 *
 * アトミック操作を使ってmutexロック変数をセットする.
 *
 * @param mutex mutexロック変数へのポインタ
 */
.type _mutex_acquire, "function"
_mutex_acquire:
loop:
    ldaxr w1, [x0]
    cmp   w1, #MUTEX_LOCKED     /* IF already locked ,*/
    beq   wait                  /* THEN try again...*/

    mov   w1, #MUTEX_LOCKED     /* ELSE unlocked, try to lock..    */
    stlxr w2, w1, [x0]
    cbnz  w2, loop
    
    dmb   sy                    /* Data Memory Barrier opcode waits for memory accesses */
                                /*    to complete before returning. */
    ret

wait:
    wfe
    b     loop

/**
 * @fn void _mutex_release(unsigned int *mutex)
 *
 * mutexロック変数をアンロックする.
 *
 * @param mutex    pointer to mutex lock variable
 */
.type _mutex_release, "function"
_mutex_release:
    mov x1, #MUTEX_UNLOCKED
    dmb sy              /* Required before accessing protected resource */

    str x1, [x0]        /* Unlock mutex */
    dsb sy

    sev
    ret

/**
 * @fn int _atomic_mutex_check(unsigned int *)
 *
 * mutexロックが利用可能であるか自動的にチェックを試みる.
 *
 * @param mutexエントリ状態のアドレス.
 * @return 成功の場合は 0, 失敗の場合は -1.
 */
.type _atomic_mutex_check, "function"
_atomic_mutex_check:
    ldaxr w1, [x0]          /* load exclusive the muxtab state     */
    cmp   w1, #MUTEX_FREE   /* IF state == MUTEX_FREE        */
    beq   _increment        /* THEN increment to MUTEX_USED        */
                            /* OTHERWISE clrex and return         */
    clrex
    dmb   sy
    mov   x0, #-1
    ret

_increment:
    add   w1, w1, #1            /* increments to MUTEX_USED        */
    stlxr w2, w1, [x0]          /* strex MUTEX_USED            */
    cbnz  w2, _atomic_mutex_check                
    
    dmb   sy
    mov   x0, #0
    ret
