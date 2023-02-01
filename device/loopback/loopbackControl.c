/**
 * @file     loopbackControl.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <loopback.h>

/**
 * @ingroup loopback
 *
 * ループバックデバイスの制御関数.
 * @param devptr デバイステーブルエントリ
 * @param func 実行する制御関数
 * @param arg1 制御関数に渡す第1引数
 * @param arg2 制御関数に渡す第2引数
 * @return 制御関数の結果
 */
devcall loopbackControl(device *devptr, int func, long arg1, long arg2)
{
    struct loopback *lbkptr = NULL;
    int old;

    lbkptr = &looptab[devptr->minor];

    /* Check if loopback is open */
    if (LOOP_STATE_ALLOC != lbkptr->state)
    {
        return SYSERR;
    }

    switch (func)
    {
    case LOOP_CTRL_SET_FLAG:
        old = lbkptr->flags;
        lbkptr->flags |= arg1;
        return old;

    case LOOP_CTRL_CLR_FLAG:
        old = lbkptr->flags & arg1;
        lbkptr->flags &= ~(ulong)arg1;
        return old;

    default:
        return SYSERR;
    }

    return OK;
}
