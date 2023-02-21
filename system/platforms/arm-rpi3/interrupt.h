/**
 * @file interrupt.h
 *
 * 割り込み処理に関する定数と定義.
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include <stddef.h>
#include "bcm2837.h"

typedef interrupt (*interrupt_handler_t)(void);

extern interrupt_handler_t interruptVector[];

typedef unsigned long irqmask;  /**< machine status for disable/restore  */


void enable(void);
irqmask disable(void);
irqmask restore(irqmask);
void enable_irq(irqmask);
void disable_irq(irqmask);

/* IRQ定義をインクルード  */



#endif /* _INTERRUPT_H_ */
