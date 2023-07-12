/**
 * @file platforminit.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

//#include <stdint.h>
//#include <stddef.h>
#include <platform.h>
#include <string.h>
//#include <framebuffer.h>
//#include <usbkbd.h>
//#include <stdio.h>
//#include <usb_util.h>
#include "bcm2837_mbox.h"
#include "bcm2837.h"
#include <rpi_gpio.h>
#include "../../../device/uart-pl011/pl011.h"
#include <mmu.h>
#include <random.h>
#include <mutex.h>
#include <queue.h>
#include <thread.h>
#include <semaphore.h>
#include <dma_buf.h>

/** ブートローダがARMブートタグを配置した物理メモリアドレス.
 * start.Sで設定される。ここでは、.bssに配置されないように
 * ダミー値で初期化する。 */
const struct atag *atags_ptr = (void*)-1;

/* カーネルの終わり（サニティチェックで使用）  */
extern void *_end;

/**
 * @ingroup bcm2837
 * GPIOピン16を出力に初期化する.
 */
void led_init(void)
{
    volatile struct rpi_gpio_regs *regptr;
    regptr = (struct rpi_gpio_regs *)(GPIO_REGS_BASE);
    regptr->gpfsel[1] &= ~(7 << 18);
    regptr->gpfsel[1] |=  (1 << 18);
}

/**
 * @ingroup bcm2837
 *
 * GPIOピン16をオン.
 */
void led_on(void)
{
    volatile struct rpi_gpio_regs *regptr = (struct rpi_gpio_regs *)(GPIO_REGS_BASE);
    regptr->gpset[0] =  (1 << 16);
}

/**
 * @ingroup bcm2837
 *
 * GPIOピン16をオフ.
 */
void led_off(void)
{
    volatile struct rpi_gpio_regs *regptr = (struct rpi_gpio_regs *)(GPIO_REGS_BASE);
    regptr->gpclr[0] =  (1 << 16);
}

/**
 * @ingroup bcm2837
 *
 * Raspberry Pi 3 B+ 固有の情報を初期化する.
 * @return OK
 */
int platforminit(void)
{
    uint32_t  __attribute__((aligned(16))) mailbuffer[8];

    strlcpy(platform.family, "BCM2837B0", PLT_STRMAX);
    strlcpy(platform.name, "Raspberry Pi 3 B+", PLT_STRMAX);
    platform.minaddr = 0;
    platform.maxaddr = (void *)0x3EFFFFFC;
    platform.clkfreq = 1000000;
    platform.serial_low = 0;
    platform.serial_high = 0;

    // maxaddr
    get_armmemory_mailbox(&mailbuffer[0]);
    platform.maxaddr = (void *)(mailbuffer[MBOX_HEADER_LENGTH + TAG_HEADER_LENGTH] + mailbuffer[MBOX_HEADER_LENGTH + TAG_HEADER_LENGTH+1]);

    /// serial_low/high
    get_serial_mailbox(&mailbuffer[0]);
    platform.serial_low = mailbuffer[MBOX_HEADER_LENGTH + TAG_HEADER_LENGTH];
    platform.serial_high = mailbuffer[MBOX_HEADER_LENGTH + TAG_HEADER_LENGTH+1];

    uint32_t cache_encoding = _getcacheinfo();  // Read CCSIDR
    switch (cache_encoding) {
        case 0x7003E01A:
            platform.dcache_size = 8;       // 8 KB
            break;
        case 0x7007E01A:
            platform.dcache_size = 16;      // 16 KB
            break;
        case 0x700FE01A:
            platform.dcache_size = 32;      // 32 KB
            break;
        case 0x701FE01A:
            platform.dcache_size = 64;      // 64 KB
            break;
        default:
            platform.dcache_size = 0;
            break;
    }
    //id.Raw32 = rpi_getModel();
    //platform.model_id = id.model;

    /* BCM2837の電源を初期化する */
    bcm2837_power_init();

    /* メモリ管理ユニットを初期化する */
    mmu_init();

    /* mutexテーブルを初期化する */
    for (int i = 0; i < NMUTEX; i++) {
        muxtab[i].state = MUTEX_FREE;
        muxtab[i].lock = MUTEX_UNLOCKED;
    }

    /* dmaバッファスペースを初期化する */
    dma_buf_init();

    /* Initialze the Hardware Random Number Generator */
    random_init();

    /* Initialize the mutexes for global tables */
    quetab_mutex = mutex_create();

    /* グローバルテーブル用のmutexを初期化する */
    register struct thrent *thrptr;
    for (int i = 0; i < NTHREAD; i++) {
        thrtab_mutex[i] = mutex_create();
        thrptr = &thrtab[i];
        thrptr->core_affinity = -1;
    }

    for (int i = 0; i < NSEM; i++) {
        semtab_mutex[i] = mutex_create();
    }

    return OK;
}
