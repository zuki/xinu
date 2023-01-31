/**
 * @file kprintf.c
 */

/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <kernel.h>
#include <stdarg.h>

/**
 * @ingroup uartgeneric
 *
 * カーネルprintf: フォーマットしてSERIAL0に同期出力する.
 *
 * @param format
 *      フォーマット文字列。 この実装では標準的なすべての書式指定子を
 *      サポートしているわけではない。サポートしている変換指定子の説明に
 *      ついては _doprnt() を参照のこと。
 * @param ...
 *      フォーマット文字列にマッチする引数
 *
 * @return
 *      書き出した文字数
 */
syscall kprintf(const char *format, ...)
{
    int retval;
    va_list ap;

    va_start(ap, format);
    retval = kvprintf(format, ap);
    va_end(ap);
    return retval;
}
