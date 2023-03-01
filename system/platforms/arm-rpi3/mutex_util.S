/**
 * @file mutex.S
 * _mutex_acquire, _mutex_release, _atomic_mutex_check を提供する.
 *
 * Written by:  Rade Latinovich
 *              Patrick McGee
 */

#include "mutex.h"

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
_mutex_acquire:
    .func    _mutex_acquire

    push  {lr}

loop:
    pldw  [r0]

    ldrex r1, [r0]              /* Load the value of the lock from memory */
    cmp   r1, #MUTEX_LOCKED     /* IF already locked ,*/
    beq   wait                  /* THEN try again...*/

    mov   r1, #MUTEX_LOCKED     /* ELSE unlocked, try to lock..    */
    strex r2, r1, [r0]
    cmp   r2, #0                /* IF lock acquire failed this time around,*/
    bne   loop                  /* THEN try again back up at the top */
    dmb                         /* Data Memory Barrier opcode waits for memory accesses */
                                /*    to complete before returning. */
    pop   {lr}
    bx    lr

wait:
    wfe
    b     loop

    .endfunc

/**
 * @fn void _mutex_release(unsigned int *mutex)
 *
 * mutexロック変数をアンロックする.
 *
 * @param mutex    pointer to mutex lock variable
 */
_mutex_release:
    .func    _mutex_release

    mov r1, #MUTEX_UNLOCKED
    dmb                 /* Required before accessing protected resource */

    str r1, [r0]        /* Unlock mutex */
    dsb

    sev
    bx  lr

    .endfunc

/**
 * @fn int _atomic_mutex_check(unsigned int *)
 *
 * mutexロックが利用可能であるか自動的にチェックを試みる.
 *
 * @param mutexエントリ状態のアドレス.
 * @return 成功の場合は 0, 失敗の場合は -1.
 */
.globl _atomic_mutex_check
_atomic_mutex_check:
    .func _atomic_mutex_check

    pldw  [r0]              /* Preload data w/ intent to write    */
    ldrex r1, [r0]          /* load exclusive the muxtab state     */
    cmp   r1, #MUTEX_FREE   /* IF state == MUTEX_FREE        */
    beq   _increment        /* THEN increment to MUTEX_USED        */
                            /* OTHERWISE clrex and return         */
    clrex
    dmb
    mov   r0, #-1
    bx    lr

_increment:
    add   r1, r1, #1            /* increments to MUTEX_USED        */
    strex r2, r1, [r0]          /* strex MUTEX_USED            */
    cmp   r2, #0                /* IF strex doesnt succeed,        */
    bne   _atomic_mutex_check   /* jump back to the top            */
                                /* OTHERWISE return as success        */
    dmb
    mov   r0, #0
    bx    lr

    .endfunc
