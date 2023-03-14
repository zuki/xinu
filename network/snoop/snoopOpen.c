/** @file snoopOpen.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <interrupt.h>
#include <network.h>
#include <snoop.h>

/**
 * @ingroup snoop
 *
 * ネットワークデバイスからのキャプチャをオープンする.
 * @param cap キャプチャ構造体へのポインタ
 * @param devname デバイスの名前、すべてのネットワークデバイスの場合は ALL
 * @return オープンに成功したら ::OK; れ以外は ::SYSERR
 * @pre capにはフィルタリング設定がなされていること
 */
int snoopOpen(struct snoop *cap, char *devname)
{
    int i;
    int count = 0;
    int devnum;
    irqmask im;

    /* 引数のエラーチェック */
    if ((NULL == cap) || (NULL == devname))
    {
        return SYSERR;
    }

    SNOOP_TRACE("Opening capture on %s", devname);

    /* 統計のリセット */
    cap->ncap = 0;
    cap->nmatch = 0;
    cap->novrn = 0;

    /* パケットをキューイングするメールボックスを割り当てる */
    cap->queue = mailboxAlloc(SNOOP_QLEN);
    if (SYSERR == (int)cap->queue)
    {
        SNOOP_TRACE("Failed to allocate mailbox");
        return SYSERR;
    }

    /* devnameが"ALL"の場合は実行中のすべてのネットワークインタフェースに
     * キャプチャを接続する */
    if (0 == strcmp(devname, "ALL"))
    {
        im = disable();
#if NNETIF
        for (i = 0; i < NNETIF; i++)
        {
            if (NET_ALLOC == netiftab[i].state)
            {
                netiftab[i].capture = cap;
                count++;
                SNOOP_TRACE("Attached capture to interface %d", i);
            }
        }
#endif
        restore(im);
        if (0 == count)
        {
            SNOOP_TRACE("Capture not attached to any interface");
            mailboxFree(cap->queue);
            return SYSERR;
        }
        return OK;
    }

    /* キャプチャを接続するネットワークインタフェースを決定する */
    devnum = getdev(devname);
    if (SYSERR == devnum)
    {
        SNOOP_TRACE("Invalid device");
        mailboxFree(cap->queue);
        return SYSERR;
    }
    im = disable();
#if NNETIF
    for (i = 0; i < NNETIF; i++)
    {
        if ((NET_ALLOC == netiftab[i].state)
            && (netiftab[i].dev == devnum))
        {
            netiftab[i].capture = cap;
            restore(im);
            SNOOP_TRACE("Attached capture to interface %d", i);
            return OK;
        }
    }
#endif

    /* No network interface found */
    restore(im);
    SNOOP_TRACE("No network interface found");
    mailboxFree(cap->queue);
    return SYSERR;
}
