/**
 * @file     clkinit.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <conf.h>

#if RTCLOCK

#include <kernel.h>
#include <stddef.h>
#include <platform.h>
#include <interrupt.h>
#include <clock.h>
#include <queue.h>

/**
 * @ingroup timer
 *
 * ::clktime 以降に発生したタイマー割り込みの数は増分される。
 * ::clkticks が ::CLKTICKS_PER_SEC に達したら、::clktime は
 * 再び増分し、::clkticks は 0 にリセットされる。
 */
volatile ulong clkticks;

/**
 * @ingroup timer
 *
 * システム起動からの経過秒数
 */
volatile ulong clktime;

/** スリープ中のプロセスのキュー  */
qid_typ sleepq;

/* TODO: Get rid of ugly x86 ifdef.  */
#ifdef _XINU_PLATFORM_X86_
extern void clockIRQ(void);
#define CLOCKBASE 0x40         /* I/O base port of clock chip for x86 */
#define CLOCKCTL (CLOCKBASE+3) /* chip CSW I/O port for x86           */
ulong time_intr_freq = 0;     /** frequency of XINU clock interrupt   */
#endif

/**
 * @ingroup timer
 *
 * クロックとスリープキューを初期化する。この関数は起動時に
 * 呼び出される。
 */
void clkinit(void)
{
    sleepq = queinit();         /* sleepキューを初期化する */

    clkticks = 0;

#ifdef DETAIL
    kprintf("Time base %dHz, Clock ticks at %dHz\r\n",
            platform.clkfreq, CLKTICKS_PER_SEC);
#endif

    /* TODO: Get rid of ugly x86 ifdef.  */
#ifdef _XINU_PLATFORM_X86_
	time_intr_freq = platform.clkfreq / CLKTICKS_PER_SEC;
	outb(CLOCKCTL, 0x34);
	/* LSB then MSB */
	outb(CLOCKBASE, time_intr_freq);
	outb(CLOCKBASE, time_intr_freq >> 8);
	outb(CLOCKBASE, time_intr_freq >> 8); /* why??? */
	set_evec(IRQBASE, (ulong)clockIRQ);
#else
    /* クロック割り込みを登録する */
    interruptVector[IRQ_TIMER] = clkhandler;
    enable_irq(IRQ_TIMER);
    clkupdate(platform.clkfreq / CLKTICKS_PER_SEC);
#endif
}

#endif                          /* RTCLOCK */
