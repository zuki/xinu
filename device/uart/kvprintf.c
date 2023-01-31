/**
 * @file kvprintf.c
 */

/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <device.h>
#include <kernel.h>
#include <stdarg.h>
#include <stdio.h>

/**
 * @ingroup uartgeneric
 *
 * カーネルprintf: フォーマットしてSERIAL0に同期出力する.
 *
 * @param format
 *      フォーマット文字列。 この実装では標準的なすべての書式指定子を
 *      サポートしているわけではない。サポートしている変換指定子の説明に
 *      ついては _doprnt() を参照のこと。
 * @param ap
 *      フォーマット文字列にマッチする引数
 *
 * @return
 *      書き出した文字数
 */
syscall kvprintf(const char *format, va_list ap)
{
    int retval;
    irqmask im;

    /* 注: グローバル割り込みを無効にする必要は厳密にはないが、割り込み
     * ハンドラからうっかり kprintf() を呼び出して kprintf() が互いの
     * つま先を踏んでしまうのを防ぐために無効にする。
     */
    im = disable();
    retval = _doprnt(format, ap, (int (*)(int, int))kputc, (int)&devtab[SERIAL0]);
    restore(im);
    return retval;
}
