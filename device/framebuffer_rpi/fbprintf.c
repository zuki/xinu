/**
 * @file kprintf.c
 */

/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <stddef.h>
#include <stdarg.h>
#include <device.h>
#include <stdio.h>
#include <framebuffer.h>

/**
 * @ingroup framebuffer
 *
 * 同期的に文字を書き出す.
 * @param c 書き出す文字
 * @param devptr 文字を書き出すデバイスへのポインタ
 * @return 成功の場合は c、失敗の場合は SYSERR
 */
syscall fbputc(uchar c, device *devptr)
{
    fbPutc(devptr, c);
    return (int)c;
}

/**
 * @ingroup framebuffer
 *
 * 同期的にカーネルを読み込む. 現在は未実装。
 * @param devptr 文字を読み込むデバイスへのポインタ
 * @return 成功の場合は読み込んだ文字、失敗の場合は SYSERR
 */
syscall fbgetc(device *devptr)
{
    return SYSERR; //not possible.
}

/**
 * @ingroup framebuffer
 *
 * カーネルprintf: フレームバッファへフォーマットした、バッファ
 * なしの出力を行う. シリアルドライバ用の"kprintf*に相当する。
 * 代替: fprintf(FRAMBUF, "string");
 * @param fmt 出力する文字列へのポインタ
 * @return 成功の場合は OK
 */
syscall fbprintf(char *fmt, ...)
{
    va_list ap;

	irqmask mask = disable();

    va_start(ap, fmt);
    _doprnt(fmt, ap, (int (*)(int, int))fbputc, (int)&devtab[FRAMEBUF]);
    va_end(ap);

	restore(mask);
    return OK;
}
