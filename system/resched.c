/**
 * @file resched.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <thread.h>
#include <stdint.h>
#include <clock.h>
#include <queue.h>
#include <memory.h>

extern void ctxsw(void *, void *, uint8_t);
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
    uint8_t asid;                 /* address space identifier */
    struct thrent *throld;      /* old thread entry */
    struct thrent *thrnew;      /* new thread entry */
    unsigned int cpuid;

    if (resdefer > 0)
    {                           /* 遅延設定されていたら、countを増分して復帰する */
        resdefer++;
        return (OK);
    }

    cpuid = getcpuid();

    thrtab_acquire(thrcurrent[cpuid]);
    throld = &thrtab[thrcurrent[cpuid]];
    throld->intmask = disable();
    if (THRCURR == throld->state)
    {
        // readylistの先頭のスレッドよりカレントスレッドの優先度が高い
        quetab_acquire();
        //kprintf("prio: %d, key: %d\n", throld->prio, firstkey(readylist[cpuid]));
        //if (nonempty(readylist[cpuid]) && (throld->prio > firstkey(readylist[cpuid])))
        if (isempty(readylist[cpuid]) || (throld->prio > firstkey(readylist[cpuid])))
        {
            // カレントスレッドを続ける
            quetab_release();
            thrtab_release(thrcurrent[cpuid]);
            restore(throld->intmask);
            return OK;
        }
        // カレントスレッドをreadylistに入れる
        quetab_release();
        throld->state = THRREADY;
        insert(thrcurrent[cpuid], readylist[cpuid], throld->prio);
    }

    thrtab_release(thrcurrent[cpuid]);

    /* readlylistから優先度がもっとも高いスレッドを取得する */
    thrcurrent[cpuid] = dequeue(readylist[cpuid]);
    thrtab_acquire(thrcurrent[cpuid]);
    thrnew = &thrtab[thrcurrent[cpuid]];
    thrnew->state = THRCURR;
    thrtab_release(thrcurrent[cpuid]);

    /* アドレス空間識別子をスレッドidに変更する: mips only, armは無視 */
    asid = thrcurrent[cpuid] & 0xff;
    //kprintf("&old [%s]: 0x%x (0x%x), &new [%s]: 0x%x (0x%x)\n", throld->name, &throld->stkptr, throld->stkptr, thrnew->name, &thrnew->stkptr, thrnew->stkptr);
    ctxsw(&throld->stkptr, &thrnew->stkptr, asid);

    /* 再開された時、もとのスレッドはここに返る */
    restore(throld->intmask);
    return OK;
}
