/**
 * @file ionull.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>

/**
 * @ingroup devcalls
 *
 * 何もしない（devtabのどうでもよいエントリで使用される）
 *
 * @return ::OK
 */
devcall ionull(void)
{
    return OK;
}
