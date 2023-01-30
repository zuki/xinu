/**
 * @file     main.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <device.h>
#include <ether.h>
#include <platform.h>
#include <shell.h>
#include <stdio.h>
#include <thread.h>
#include <version.h>

static void print_os_info(void);

/**
 * @ingroup boot
 *
 * メインスレッド.  この関数を変更することによりEmbedded Xinuの起動時に行う
 * ことをカスタマイズすることができる。デフォルトでは構成されたデバイスと
 * 機能に基づいて、すべてのプラットフォームで妥当な動作をするように設計されて
 * いる。
 */
thread main(void)
{
#if HAVE_SHELL
    int shelldevs[4][3];
    uint nshells = 0;
#endif

    /* オペレーティングシステムに関する情報を表示する  */
    print_os_info();

    /* すべてのethernetデバイスをオープンする */
#if NETHER
    {
        uint i;

        for (i = 0; i < NETHER; i++)
        {
            if (SYSERR == open(ethertab[i].dev->num))
            {
                kprintf("WARNING: Failed to open %s\r\n",
                        ethertab[i].dev->name);
            }
        }
    }
#endif /* NETHER */

    /* 最初のTTY (CONSOLE) を設定する  */
#if defined(CONSOLE) && defined(SERIAL0)
    if (OK == open(CONSOLE, SERIAL0))
    {
  #if HAVE_SHELL
        shelldevs[nshells][0] = CONSOLE;
        shelldevs[nshells][1] = CONSOLE;
        shelldevs[nshells][2] = CONSOLE;
        nshells++;
  #endif
    }
    else
    {
        kprintf("WARNING: Can't open CONSOLE over SERIAL0\r\n");
    }
#elif defined(SERIAL0)
  #warning "No TTY for SERIAL0"
#endif

    /* 可能であれば2番めのTTY (TTY1) を設定する */
#if defined(TTY1)
  #if defined(KBDMON0)
    /* TTY1をキーボードに関連させ、フレームバッファを出力に使用する  */
    if (OK == open(TTY1, KBDMON0))
    {
    #if HAVE_SHELL
        shelldevs[nshells][0] = TTY1;
        shelldevs[nshells][1] = TTY1;
        shelldevs[nshells][2] = TTY1;
        nshells++;
    #endif
    }
    else
    {
        kprintf("WARNING: Can't open TTY1 over KBDMON0\r\n");
    }
  #elif defined(SERIAL1)
    /* TTY1をSERIAL1に関連付ける  */
    if (OK == open(TTY1, SERIAL1))
    {
    #if HAVE_SHELL
        shelldevs[nshells][0] = TTY1;
        shelldevs[nshells][1] = TTY1;
        shelldevs[nshells][2] = TTY1;
        nshells++;
    #endif
    }
    else
    {
        kprintf("WARNING: Can't open TTY1 over SERIAL1\r\n");
    }
  #endif /* SERIAL1 */
#else /* TTY1 */
  #if defined(KBDMON0)
    #warning "No TTY for KBDMON0"
  #elif defined(SERIAL1)
    #warning "No TTY for SERIAL1"
  #endif
#endif /* TTY1 */

    /* シェルを開始する  */
#if HAVE_SHELL
    {
        uint i;
        char name[16];

        for (i = 0; i < nshells; i++)
        {
            sprintf(name, "SHELL%u", i);
            if (SYSERR == ready(create
                                (shell, INITSTK, INITPRIO, name, 3,
                                 shelldevs[i][0],
                                 shelldevs[i][1],
                                 shelldevs[i][2]),
                                RESCHED_NO))
            {
                kprintf("WARNING: Failed to create %s", name);
            }
        }
    }
#endif

    return 0;
}

/* （リンカが提供する）メモリ内のカーネルのstart */
extern void _start(void);

static void print_os_info(void)
{
    kprintf(VERSION);
    kprintf("\r\n\r\n");

#ifdef DETAIL
    /* 検知したプラットフォームを出力する */
    kprintf("Processor identification: 0x%08X\r\n", cpuid);
    kprintf("Detected platform as: %s, %s\r\n\r\n",
            platform.family, platform.name);
#endif

    /* Xinuのメモリレイアウトを出力する */
    kprintf("%10d bytes physical memory.\r\n",
            (ulong)platform.maxaddr - (ulong)platform.minaddr);
#ifdef DETAIL
    kprintf("           [0x%08X to 0x%08X]\r\n",
            (ulong)platform.minaddr, (ulong)(platform.maxaddr - 1));
#endif


    kprintf("%10d bytes reserved system area.\r\n",
            (ulong)_start - (ulong)platform.minaddr);
#ifdef DETAIL
    kprintf("           [0x%08X to 0x%08X]\r\n",
            (ulong)platform.minaddr, (ulong)_start - 1);
#endif

    kprintf("%10d bytes Xinu code.\r\n", (ulong)&_etext - (ulong)_start);
#ifdef DETAIL
    kprintf("           [0x%08X to 0x%08X]\r\n",
            (ulong)_start, (ulong)&_end - 1);
#endif

    kprintf("%10d bytes stack space.\r\n", (ulong)memheap - (ulong)&_end);
#ifdef DETAIL
    kprintf("           [0x%08X to 0x%08X]\r\n",
            (ulong)&_end, (ulong)memheap - 1);
#endif

    kprintf("%10d bytes heap space.\r\n",
            (ulong)platform.maxaddr - (ulong)memheap);
#ifdef DETAIL
    kprintf("           [0x%08X to 0x%08X]\r\n\r\n",
            (ulong)memheap, (ulong)platform.maxaddr - 1);
#endif
    kprintf("\r\n");
}
