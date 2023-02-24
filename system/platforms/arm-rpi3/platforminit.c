/**
 * @file platforminit.c
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <stdint.h>
#include <stddef.h>
#include <platform.h>
#include <string.h>
#include <framebuffer.h>
#include <usbkbd.h>
#include <stdio.h>
#include <usb_util.h>
#include "rpi-mailbox.h"
#include "bcm2837.h"

const struct atag *atags_ptr = (void*)-1;

/* -------------------------------------------------------------------------
 *  GPIO FSEL定義 ... BCM2835.PDF マニュアル p.92 参照
 * ------------------------------------------------------------------------- */
typedef enum {
    GPIO_INPUT    = 0b000,              // 0
    GPIO_OUTPUT   = 0b001,              // 1
    GPIO_ALTFUNC5 = 0b010,              // 2
    GPIO_ALTFUNC4 = 0b011,              // 3
    GPIO_ALTFUNC0 = 0b100,              // 4
    GPIO_ALTFUNC1 = 0b101,              // 5
    GPIO_ALTFUNC2 = 0b110,              // 6
    GPIO_ALTFUNC3 = 0b111,              // 7
} GPIOMODE;


/**
 * Raspberry Pi 3B+ 固有の情報を初期化する.
 * @return OK
 */
int platforminit(void)
{
    uint64_t serial;
    RPI_MODEL_ID id;

    strlcpy(platform.family, "BCM2837B0", PLT_STRMAX);
    strlcpy(platform.name, "Raspberry Pi 3 B+", PLT_STRMAX);
    platform.minaddr = 0;
    platform.maxaddr = (void *)rpi_LastARMAddr();
    platform.clkfreq = 1000000;
    serial = rpi_getserial();
    platform.serial_low = (uint32_t)serial;
    platform.serial_high = (uint32_t)(serial >> 32);
    id.Raw32 = rpi_getModel();
    platform.model_id = id.model;

    /* BCM2837の電源を初期化する */
    bcm2837_power_init();

    return OK;
}
