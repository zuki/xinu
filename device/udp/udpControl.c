/**
 * @file     udpControl.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <stdlib.h>
#include <device.h>
#include <network.h>
#include <udp.h>

/**
 * @ingroup udpexternal
 *
 * UDPデバイスの制御関数.
 * @param devptr UDPデバイステーブルエントリ
 * @param func 実行する制御関数
 * @param arg1 制御関数への第1引数
 * @param arg2 制御関数への第2引数
 * @return 制御関数の結果
 */
devcall udpControl(device *devptr, int func, long arg1, long arg2)
{
    struct udp *udpptr;
    uchar old;

    udpptr = &udptab[devptr->minor];

    switch (func)
    {
    case UDP_CTRL_ACCEPT:
        /* arg1はポート番号、arg2はnetaddrへのポインタ */
        udpptr->localpt = arg1;
        if (NULL == arg2)
        {
            return SYSERR;
        }
        else
        {
            netaddrcpy(&(udpptr->localip), (struct netaddr *)arg2);
        }
        return OK;
    case UDP_CTRL_BIND:
        /* arg1はポート番号、arg2はnetaddrへのポインタ */
        udpptr->remotept = arg1;
        if (NULL == arg2)
        {
            bzero(&(udpptr->remoteip), sizeof(struct netaddr));
        }
        else
        {
            netaddrcpy(&(udpptr->remoteip), (struct netaddr *)arg2);
        }
        return OK;
    case UDP_CTRL_CLRFLAG:
        /* arg1はクリアするフラグ */
        old = udpptr->flags & arg1;
        udpptr->flags &= ~arg1;
        return old;
    case UDP_CTRL_SETFLAG:
        /* arg1はセットするフラグ */
        old = udpptr->flags & arg1;
        udpptr->flags |= arg1;
        return old;
    default:
        return SYSERR;
    }

    return OK;
}
