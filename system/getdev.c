/**
 * @file     getdev.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <string.h>

/**
 * @ingroup devcalls
 *
 * デバイス名をデバイス番号に変換する
 *
 * @param dev
 *      デバイス名
 *
 * @return
 *      デバイス番号、デバイスが見つからなかった場合は ::SYSERR
 */
syscall getdev(const char *dev)
{
    int devnum;

    for (devnum = 0; devnum < NDEVS; devnum++)
    {
        if (0 == strncmp(dev, devtab[devnum].name, DEVMAXNAME))
        {
            return devnum;
        }
    }

    return SYSERR;
}
