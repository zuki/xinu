/**
 * @file resched.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>
#include <clock.h>
#include <queue.h>
#include <memory.h>

extern void ctxsw(void *, void *, uchar);
int resdefer;                   /* 再スケジュールが遅延されたら >0 */

/**
 * @ingroup threads
 *
 * プロセッサを最も優先度の高いreadyスレッドにリスケジュールする。
 * この関数が呼び出された際、thrcurrentには現在のスレッドIDが、
 * Threadtab[thrcurrent].pstateにはTHRREADYでなければ現在の
 * スレッドの正しいNEXT状態が設定されている。
 * @return スレッドがコンテキストスイッチバックされたら OK
 */
int resched(void)
{
    uchar asid;                 /* address space identifier */
    struct thrent *throld;      /* old thread entry */
    struct thrent *thrnew;      /* new thread entry */

    if (resdefer > 0)
    {                           /* 遅延されたら、countを増分して復帰 */
        resdefer++;
        return (OK);
    }

    throld = &thrtab[thrcurrent];

    throld->intmask = disable();

    if (THRCURR == throld->state)
    {
        // readylistの先頭のスレッドよりカレントスレッドの優先度が高い
        if (nonempty(readylist) && (throld->prio > firstkey(readylist)))
        {
            // カレントスレッドを続ける
            restore(throld->intmask);
            return OK;
        }
        // カレントスレッドをreadylistに入れる
        throld->state = THRREADY;
        insert(thrcurrent, readylist, throld->prio);
    }

    /* readlylistから優先度がもっとも高いスレッドを取得する */
    thrcurrent = dequeue(readylist);
    thrnew = &thrtab[thrcurrent];
    thrnew->state = THRCURR;

    /* アドレス空間識別子をスレッドidに変更する: mips only, armは無視 */
    asid = thrcurrent & 0xff;
    ctxsw(&throld->stkptr, &thrnew->stkptr, asid);

    /* 再開された時、もとのスレッドはここに返る */
    restore(throld->intmask);
    return OK;
}
