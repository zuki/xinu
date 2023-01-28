/**
 * @file arpLookup.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <arp.h>
#include <clock.h>
#include <interrupt.h>
#include <string.h>
#include <thread.h>

/**
 * @ingroup arp
 *
 * ARPテーブルから指定したプロトコルアドレスのハードウェア
 * アドレスを取得する。
 * @param netptr ネットワークインタフェース
 * @param praddr プロトコルアドレス
 * @param hwaddr ハードウェアアドレスを格納するバッファ
 * @return ハードウェアが取得できたら OK、そうでなければ TIMEOUT か SYSERR
 */
syscall arpLookup(struct netif *netptr, const struct netaddr *praddr,
                  struct netaddr *hwaddr)
{
    struct arpEntry *entry = NULL;  /**< ARPテーブルエントリへのポインタ   */
    uint lookups = 0;               /**< ARP検索施行回数             */
    int ttl;                        /**< ARPテーブルエントリのTTL     */
    irqmask im;                     /**< 割り込みの状態               */

    /* ポインタのエラーチェック */
    if ((NULL == netptr) || (NULL == praddr) || (NULL == hwaddr))
    {
        ARP_TRACE("Invalid args");
        return SYSERR;
    }

    ARP_TRACE("Looking up protocol address");

    /* ARPテーブルから次のいずれかまで宛先のハードウェアアドレスの
     * 取得を試みる:
     * 1) 検索成功; 2) TIMEOUT発生; 3) SYSERR発生;
     * 4) 最大検索思考回数になる. */
    while (lookups < ARP_MAX_LOOKUP)
    {
        lookups++;

        /* ARPテーブルからエントリを取得する */
        im = disable();
        entry = arpGetEntry(praddr);

        /* ARPエントリが存在しない場合は未解決のエントリを作成する */
        if (NULL == entry)
        {
            ARP_TRACE("Entry does not exist");
            entry = arpAlloc();
            if (SYSERR == (int)entry)
            {
                restore(im);
                return SYSERR;
            }

            entry->state = ARP_UNRESOLVED;
            entry->nif = netptr;
            netaddrcpy(&entry->praddr, praddr);
            entry->expires = clktime + ARP_TTL_UNRESOLVED;
            entry->count = 0;
        }

        /* エントリが解決されたらハードウェアアドレスをバッファにコピーする */
        if (ARP_RESOLVED == entry->state)
        {
            netaddrcpy(hwaddr, &entry->hwaddr);
            ARP_TRACE("Entry exists");
            return OK;
        }

        /* エントリが未解決の場合、スレッドをエンキューして解決を待機 */
        if (entry->count >= ARP_NTHRWAIT)
        {
            restore(im);
            ARP_TRACE("Queue of waiting threads is full");
            return SYSERR;
        }
        entry->waiting[entry->count] = gettid();
        entry->count++;
        ttl = (entry->expires - clktime) * CLKTICKS_PER_SEC;
        restore(im);

        /* ARPリクエスを送信してレスポンスを待つ */
        if (SYSERR == arpSendRqst(entry))
        {
            ARP_TRACE("Failed to send request");
            return SYSERR;
        }
        recvclr();
        switch (recvtime(ttl))
        {
        case TIMEOUT:
        case SYSERR:
            return SYSERR;
        case ARP_MSG_RESOLVED:
        default:
            /* 応答受信、アドレス解決、検索の再試行 */
            continue;
        }
    }

    return SYSERR;
}
