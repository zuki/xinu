/**
 * @file ioerr.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>

/**
 * @ingroup devcalls
 *
 * 無条件にエラーを返す（devtabの "error" エントリで使用される）
 *
 * @return ::SYSERR
 */
devcall ioerr(void)
{
    return SYSERR;
}
