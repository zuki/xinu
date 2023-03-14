/**
 * @file telnetInit.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <telnet.h>
#include <stdlib.h>

/** @ingroup telnet
 * @var telnettab
 * @brief Telnetテーブル変数
 */
struct telnet telnettab[NTELNET];

/**
 * @ingroup telnet
 *
 * TELNET構造体を初期化する.
 * @param devptr TELNETデバイステーブルエントリ
 * @return デバイスが初期化されたら ::OK
 */
devcall telnetInit(device *devptr)
{
    struct telnet *tntptr;

    tntptr = &telnettab[devptr->minor];
    bzero(tntptr, sizeof(struct telnet));
    if (0 == devptr->minor)
    {
        tntptr->killswitch = semcreate(0);
    }
    else
    {
        tntptr->killswitch = telnettab[0].killswitch;
    }

    return OK;
}
