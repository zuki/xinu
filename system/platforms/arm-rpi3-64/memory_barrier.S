/**
 * @file memory_barrier.S
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

.globl dmb
/**
 * @fn void dmb(void)
 *
 * Data memory barrier. */
dmb:
    dmb sy
    ret
