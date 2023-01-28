/**
 * @file pause.S
 *
 * @ingroup threads
 * プロセスをアイドリング状態にするプラットフォーム固有のコード
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

.globl pause

pause:
	.func pause
	wfi               /* ARMの "wait for interrupt" 命令 */
	mov pc, lr
	.endfunc
