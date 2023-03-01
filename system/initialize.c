/**
 * @file initialize.c
 *
 * C環境の確立後に初期化を開始する。初期化後、NULLスレッドは常に
 * 準備完了（THRREADY）または実行中（THRCURR）状態を維持する
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <xinu.h>
#include <stdint.h>
#include <platform.h>
#include <mutex.h>

#ifdef WITH_USB
#include <usb_subsystem.h>
#include <usb_core_driver.h>
#include "../device/lan7800/lan7800.h"
#endif

#include "platforms/arm-rpi3/mmu.h"
#include <dma_buf.h>

/* 関数プロトタイプ */
extern thread main(void);       /* mainは最初に作成されるスレッド */
static int sysinit(void);       /* システム構造体を初期化する     */

/* 主なカーネル変数の宣言 */
struct thrent thrtab[NTHREAD];  /* スレッドテーブル               */
struct sement semtab[NSEM];     /* セマフォテーブル               */
struct monent montab[NMON];     /* モニターテーブル               */
qid_typ readylist[NCORES];      /* READYスレッドリスト            */
struct memblock memlist;        /* フリーメモリブロックリスト     */
struct bfpentry bfptab[NPOOL];  /* メモリバッファプールリスト     */

/* 主なマルチコア変数の宣言 */
mutex_t quetab_mutex;           /* quetabを守るmutex */
mutex_t thrtab_mutex[NTHREAD];  /* thrtabを守るmutex */
mutex_t semtab_mutex[NSEM];     /* semtabを守るmutex */
mutex_t serial_lock;            /* serialのロック */

static void core_nulluser(void);

/* アクティブなシステムステータス */
int thrcount;                   /* 生きているユーザスレッド数     */
tid_typ thrcurrent[NCORES];     /* 現在実行中のスレッドのID       */

/* startup.S でセットされるパラメタ */
void *memheap;                  /* ヒープの底（O/Sスタックのトップ） */
ulong cpuid;                    /* プロセッサID                      */
struct platform platform;       /* プラットフォーム固有の構成        */

/**
 * @ingroup boot
 *
 * システムを初期化して、NULLスレッドとなる.
 *
 * C環境の確立後にシステムが開始する地点である。割り込みは初期状態では
 * "無効"になっており、最終的には明示的に有効にする必要がある。この関数は
 * 初期化後、自身をNULLスレッドにする。NULLスレッドは常に実行可能な状態を
 * 維持しなければならないので、サスペンド、セマフォ待ち、スリープ、終了などの
 * 原因となるコードを実行することはできない。特に、同期出力用の kprintf を
 * 使用しない限り、I/Oを行ってはならない。
 */
void nulluser(void)
{
    /* プラットフォーム固有の初期化を行う  */
    platforminit();

    /* 一般的な初期化を行う  */
    sysinit();

    /* セカンダリコアの待機解除 */
    unparkcore(1, (void *) core_nulluser, NULL);
    unparkcore(2, (void *) core_nulluser, NULL);
    unparkcore(3, (void *) core_nulluser, NULL);

	kprintf("\r\n***********************************************************\r\n");
	kprintf("******************** Hello Xinu World! ********************\r\n");
	kprintf("***********************************************************\r\n");

    /* 割り込みを有効にする  */
    enable();

    /* メインスレッドを起動する  */
    ready(create(main, INITSTK, INITPRIO, "MAIN", 0), RESCHED_YES, CORE_ZERO);

    /* NULLスレッドは他にすることはないが終了することはできない  */
    while (TRUE)
    {
#ifndef DEBUG
        pause();
#endif                          /* DEBUG */
    }
}

/**
 * @ingroup boot
 *
 * Xinuのすべてのデータ構造とデバイスを初期化する.
 *
 * @return すべての初期化が成功したら OK
 */
static int sysinit(void)
{
    int i;
    struct thrent *thrptr;      /* thread control block pointer  */
    struct memblock *pmblock;   /* memory block pointer          */

    /* シリアルロックを初期化する */
    serial_lock = mutex_create();

    /* システムテム変数を初期化する */
    /* このNULLTHREADをシステムの最初のスレッドとしてカウントする */
    thrcount = NCORES;          /* コアごとに1つ塗るスレッド */

    /* フリーメモリリストを初期化する */
    memheap = roundmb(memheap);
    platform.maxaddr = truncmb(platform.maxaddr);
    memlist.next = pmblock = (struct memblock *)memheap;
    memlist.length = (uint)(platform.maxaddr - memheap);
    pmblock->next = NULL;
    pmblock->length = (uint)(platform.maxaddr - memheap);

    /* スレッドテーブルを初期化する */
    for (i = 0; i < NTHREAD; i++)
    {
        thrtab[i].state = THRFREE;
    }

    /* NULLスレッドエントリを初期化する*/
    thrptr = &thrtab[NULLTHREAD];
    thrptr->state = THRCURR;
    thrptr->prio = 0;
    strlcpy(thrptr->name, "prnull", TNMLEN);
    thrptr->stkbase = (void *)&_end;
    thrptr->stklen = 8192;  /* NULLSTK */
    thrptr->stkptr = 0;
    thrptr->memlist.next = NULL;
    thrptr->memlist.length = 0;
    thrptr->core_affinity = CORE_ZERO;
    thrcurrent[CORE_ZERO] = NULLTHREAD;

    /* コア1のNULLスレッド */
    thrptr = &thrtab[NULLTHREAD1];
    thrptr->state = THRCURR;
    thrptr->prio = 0;
    strlcpy(thrptr->name, "prnull01", TNMLEN);
    thrptr->stkbase = (void *)(&_end + 8192);
    thrptr->stklen = 8192;  /* NULLSTK */
    thrptr->stkptr = 0;
    thrptr->memlist.next = NULL;
    thrptr->memlist.length = 0;
    thrptr->core_affinity = CORE_ONE;
    thrcurrent[CORE_ONE] = NULLTHREAD1;

    /* コア2のNULLスレッド */
    thrptr = &thrtab[NULLTHREAD2];
    thrptr->state = THRCURR;
    thrptr->prio = 0;
    strlcpy(thrptr->name, "prnull02", TNMLEN);
    thrptr->stkbase = (void *)(&_end + 16384);
    thrptr->stklen = 8192;  /* NULLSTK */
    thrptr->stkptr = 0;
    thrptr->memlist.next = NULL;
    thrptr->memlist.length = 0;
    thrptr->core_affinity = CORE_TWO;
    thrcurrent[CORE_TWO] = NULLTHREAD2;

    /* コア3のNULLスレッド */
    thrptr = &thrtab[NULLTHREAD3];
    thrptr->state = THRCURR;
    thrptr->prio = 0;
    strlcpy(thrptr->name, "prnull03", TNMLEN);
    thrptr->stkbase = (void *)(&_end + 24576);
    thrptr->stklen = 8192;  /* NULLSTK */
    thrptr->stkptr = 0;
    thrptr->memlist.next = NULL;
    thrptr->memlist.length = 0;
    thrptr->core_affinity = CORE_THREE;
    thrcurrent[CORE_THREE] = NULLTHREAD3;

    /* セマフォを初期化する */
    for (i = 0; i < NSEM; i++)
    {
        semtab[i].state = SFREE;
        semtab[i].queue = queinit();
    }

    /* モニターを初期化する */
    for (i = 0; i < NMON; i++)
    {
        montab[i].state = MFREE;
    }

    /* バッファプールを初期化する */
    for (i = 0; i < NPOOL; i++)
    {
        bfptab[i].state = BFPFREE;
    }

    /* スレッドreadylistを初期化する */
    for (i = 0; i < NCORES; i++)
    {
        readylist[i] = queinit();
    }


#if SB_BUS
    backplaneInit(NULL);
#endif                          /* SB_BUS */

#if RTCLOCK
    /* リアルタイムクロックを初期化する */
    clkinit();
#endif                          /* RTCLOCK */

#ifdef UHEAP_SIZE
    /* ユーザメモリマネージャを初期化する */
    {
        void *userheap;             /* ユーザメモリヒープへのポインタ */
        userheap = stkget(UHEAP_SIZE);
        if (SYSERR != (int)userheap)
        {
            userheap = (void *)((uint)userheap - UHEAP_SIZE + sizeof(int));
            memRegionInit(userheap, UHEAP_SIZE);

            /* メモリプロテクションを初期化する */
            safeInit();

            /* カーネルページマッピングを初期化する */
            safeKmapInit();
        }
    }
#endif

#if USE_TLB
    /* TLBを初期化する */
    tlbInit();
    /* システムコールハンドラを登録する */
    exceptionVector[EXC_SYS] = syscall_entry;
#endif                          /* USE_TLB */

#if NMAILBOX
    /* メールボックスを初期化する */
    mailboxInit();
#endif

#if NDEVS
    /* デバイスを初期化する */
    for (i = 0; i < NDEVS; i++)
    {
        devtab[i].init((device*)&devtab[i]);
    }
#endif

#ifdef WITH_USB
    /* USBを初期化する */
    usbinit();
#endif

#if NVRAM
    /* NVRAMを初期化する */
    nvramInit();
#endif

#if NNETIF
    /* ネットインタフェースを初期化する */
    netInit();
#endif

#if GPIO
    /* LEDを初期化する */
    gpioLEDOn(GPIO_LED_CISCOWHT);
#endif
    return OK;
}

static void core_nulluser(void)
{
    unsigned int cpuid;
    cpuid = getcpuid();

    disable();
    while (TRUE)
    {
        pldw(&quetab[readylist[cpuid]].next);
        if (nonempty(readylist[cpuid]))
            resched();
    }
}
