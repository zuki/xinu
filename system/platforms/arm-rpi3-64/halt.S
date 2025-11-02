/**
 * @file halt.S
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

.globl halt
/**
 * @fn void halt(void)
 *
 * プロセッサを無限ループに送り込むことによりシステムを停止する.
 */
.type halt, "function"
halt:
    
_loop:
    b _loop
