/**
 * @file telnetControl.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <telnet.h>
#include <tty.h>

/**
 * @ingroup telnet
 *
 * telnet疑似デバイス用の制御関数.
 * @param devptr TELNETデバイステーブルエントリ
 * @param func 実行する制御関数
 * @param arg1 制御関数に渡す第1引数
 * @param arg2 制御関数に渡す第2引数
 * @return 制御関数の結果
 */
devcall telnetControl(device *devptr, int func, long arg1, long arg2)
{
    struct telnet *tntptr;
    device *phw;

    /* Setup and error check pointers to structures */
    tntptr = &telnettab[devptr->minor];
    phw = tntptr->phw;
    if (NULL == phw)
    {
        return SYSERR;
    }

    switch (func)
    {
    case TELNET_CTRL_FLUSH:
        telnetFlush(devptr);
        return OK;
    case TELNET_CTRL_CLRFLAG:
        /* 第1引数がクリアするフラグ */
        tntptr->flags &= ~arg1;
        return OK;
    case TELNET_CTRL_SETFLAG:
        /* 第1引数がセットするフラグ */
        tntptr->flags |= arg1;
        return OK;
    case TTY_CTRL_SET_IFLAG:
        return OK;
    case TTY_CTRL_CLR_IFLAG:
        return OK;
    case TTY_CTRL_SET_OFLAG:
        return OK;
    case TTY_CTRL_CLR_OFLAG:
        return OK;
    }

    return SYSERR;
}
