/**
 * @file arpNotify.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <arp.h>
#include <interrupt.h>
#include <stdlib.h>
#include <thread.h>

/**
 * @ingroup arp
 *
 * ARPテーブルエントリの解決を待っているスレッドにメッセージを送信する。
 * @param entry ARPテーブルエントリ
 * @param msg 待機スレッドに送信するメッセージ
 * @return 成功したら OK、エラーが発生したら SYSERR
 */
syscall arpNotify(struct arpEntry *entry, message msg)
{
    int i = 0;                      /**< ARPテーブルを走査するインデックス         */
    irqmask im;                     /**< 割り込み状態              */

    /* ポインタのエラーチェック */
    if (NULL == entry)
    {
        return SYSERR;
    }

    /* 各大気スレッドにメッセージを送信する */
    im = disable();
    for (i = 0; i < entry->count; i++)
    {
        if (SYSERR == send(entry->waiting[i], msg))
        {
            restore(im);
            return SYSERR;
        }
    }

    /* 待機スレッドリストをクリアする */
    entry->count = 0;
    bzero(entry->waiting, sizeof(tid_typ) * ARP_NTHRWAIT);

    restore(im);
    return OK;
}
