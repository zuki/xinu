/**
 * @file usb_subsystem.h
 * @ingroup usb
 *
 * オペレーティングシステムの他のコードからUSBサブシステムへの
 * インタフェース.
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#ifndef _USB_SUBSSYSTEM_H_
#define _USB_SUBSSYSTEM_H_

syscall usbinit(void);
syscall usbinfo(void);

#endif /* _USB_SUBSSYSTEM_H_ */
