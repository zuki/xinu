/**
 * @file mailboxAlloc.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <mailbox.h>
#include <memory.h>

/**
 * @ingroup mailbox
 *
 * 指定された数までの未処理メッセージを許容するメールボックスを
 * 割り当てる。
 *
 * @param count
 *      メールボックスが許容する最大のメッセージ数
 *
 * @return
 *      新しく割り当てられたメールボックスのインデックス、
 *      すべてのメールボックスがすでに使用されている、または、
 *      その他のリソースが割り当てられなかった場合は ::SYSERR
 */
syscall mailboxAlloc(uint count)
{
    static uint nextmbx = 0;
    uint i;
    struct mbox *mbxptr;
    int retval = SYSERR;

    /* 他のスレッドがメールボックステーブルの編集を終えるまで待機する */
    wait(mboxtabsem);

    /* 空メールボックスが見つけかるまですべてのメールボックスを調べる */
    for (i = 0; i < NMAILBOX; i++)
    {
        nextmbx = (nextmbx + 1) % NMAILBOX;
        mbxptr = &mboxtab[nextmbx];

        /* 空メールボックスが見つかったら設定して返す */
        if (MAILBOX_FREE == mbxptr->state)
        {
            /* メッセージキューのためのメモリを取得する */
            mbxptr->msgs = memget(sizeof(int) * count);

            /* メモリが割り当てられたかチェックする */
            if (SYSERR == (int)mbxptr->msgs)
            {
                break;
            }

            /* メールボックスの内容とセマフォを初期化する */
            mbxptr->count = 0;
            mbxptr->start = 0;
            mbxptr->max = count;
            mbxptr->sender = semcreate(count);
            mbxptr->receiver = semcreate(0);
            if ((SYSERR == (int)mbxptr->sender) ||
                (SYSERR == (int)mbxptr->receiver))
            {
                memfree(mbxptr->msgs, sizeof(int) * (mbxptr->max));
                semfree(mbxptr->sender);
                semfree(mbxptr->receiver);
                break;
            }

            /* このメッセージボックスを使用済みとマークする */
            mbxptr->state = MAILBOX_ALLOC;

            /* 返り値は割り当てたメールボックのインデックス */
            retval = nextmbx;
            break;
        }
    }

    /* このスレッドがメールボックテーブルの編集を終えたことを通知する */
    signal(mboxtabsem);

    /* SYSERRか割り当てたメールボックスのインデックスを返す */
    return retval;
}
