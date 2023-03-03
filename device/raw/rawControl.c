/**
 * @file rawControl.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <raw.h>
#include <interrupt.h>

/**
 * @ingroup raw
 *
 * rawソケットのためのコントロール関数.
 * @param devptr RAWデバイステーブルエントリ
 * @param func 実行するコントロール関数
 * @param arg1 コントロール関数への第一引数
 * @param arg2 コントロール関数への第二引数
 * @return コントロール関数の結果
 */
devcall rawControl(device *devptr, int func, long arg1, long arg2)
{
    struct raw *rawptr;
    uchar old;
    irqmask im;

    rawptr = &rawtab[devptr->minor];
    im = disable();
    /* Check if raw socket is open */
    if (RAW_ALLOC != rawptr->state)
    {
        restore(im);
        return SYSERR;
    }

    switch (func)
    {
        /* Set flags: arg1 = flags to set      */
        /* return old value of flags                 */
    case RAW_CTRL_SETFLAG:
        old = rawptr->flags & arg1;
        rawptr->flags |= (arg1);
        restore(im);
        return old;

        /* Clear flags: arg1 = flags to clear  */
        /* return old value of flags                 */
    case RAW_CTRL_CLRFLAG:
        old = rawptr->flags & arg1;
        rawptr->flags &= ~(arg1);
        restore(im);
        return old;
    }

    restore(im);
    return SYSERR;
}
